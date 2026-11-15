// HUMAN_MANAGER.CPP: the human-manager table and its records: construction, setup for one human,
// destruction (human_manager_lifecycle.hpp).
#include "human_manager_lifecycle.hpp"
#include "human_manager_detail.hpp"

namespace cm::aot::human_manager {
namespace {
// Cells of the table of contents and strings of the unit's pool used only here.
constexpr std::int32_t first_cd_name = -0x549c, second_cd_name = -0x5498,  // two Pascal strings of the program
    slot_constructor = -0x73e8, slot_destructor = -0x73e4;                 // vectors of a record's five tactic slots
constexpr std::uint32_t insert_title = 0x00, insert_text = 0x0a;            // pool offsets of the CD dialog's texts
constexpr std::uint32_t make_file_spec = 0x1077fee0, copy_text = 0x10727890, show_message = 0x10769b60,
                        construct_objects = 0x10773320, destroy_objects = 0x10773420, release = 0x107735a0,
                        release_entries = 0x10775170, destroy_offer = 0x10411f10, copy_bytes = 0x10771110,
                        copy_text_bytes = 0x1077dc10, fill_bytes = 0x10778300, folder_path = 0x10746300,
                        tactics_template = 0x103af480, initialize_contracts = 0x103721f0,
                        initialize_screens = 0x10064030, initialize_formation = 0x10110c80,
                        read_file = 0x1014a300;
constexpr std::uint32_t tactic_slot_size = record::tactic_size;
}

std::uint32_t construct_record(Cpu& c, std::uint32_t human) {
    const Frame frame(c, 0x10145e10, 0x40, {});
    // The original parks its argument in the caller's frame (stw r3,0x58(r1)) and reloads it.
    c.put32(frame.at(0x58), human);
    invoke(c, construct_objects, 0x10145e38, {c.u32(frame.at(0x58)), c.toc(slot_constructor), c.toc(slot_destructor),
                                              tactic_slot_size, record::tactic_slots});
    invoke(c, initialize_contracts, 0x10145e48, {c.u32(frame.at(0x58)) + record::contracts});
    invoke(c, initialize_screens, 0x10145e58, {c.u32(frame.at(0x58)) + record::screens});
    return c.u32(frame.at(0x58));
}

std::uint32_t destroy_record(Cpu& c, std::uint32_t human, std::uint32_t flag) {
    const Frame frame(c, 0x10145d70, 0x40, {.first_gpr = 30});
    c.set({{30, human}, {31, flag}});
    if (human != 0) {
        invoke(c, destroy_objects, 0x10145da0, {human, c.toc(slot_destructor), tactic_slot_size, record::tactic_slots});
        if (std::int16_t(flag) > 0) invoke(c, release, 0x10145db0, {human});
    }
    return human;
}

std::uint32_t construct_tactic(Cpu& c, std::uint32_t tactic) {
    const Frame frame(c, 0x10145e70, 0x40, {.first_gpr = 31});
    c.set(31, tactic);
    invoke(c, initialize_formation, 0x10145e88, {tactic});
    return tactic;
}

std::uint32_t destroy_tactic(Cpu& c, std::uint32_t tactic, std::uint32_t flag) {
    const Frame frame(c, 0x10145dd0, 0x40, {.first_gpr = 31});
    c.set(31, tactic);
    if (tactic != 0 && std::int16_t(flag) > 0) invoke(c, release, 0x10145df4, {tactic});
    return tactic;
}

std::uint32_t destroy_table(Cpu& c, std::uint32_t humans, std::uint32_t flag) {
    const Frame frame(c, 0x10145ea0, 0x50, {.first_gpr = 28});
    c.set({{29, humans}, {30, flag}});
    if (humans == 0) return humans;
    if (c.u32(humans + table::records) != 0) {
        std::uint32_t offset = 0;
        std::int32_t index = 0;
        c.set({{31, 0}, {28, 0}});
        // The offsets are read from the table again after each call, as the original does.
        const auto destroy_offer_at = [&](std::uint32_t field, std::uint32_t back) {
            const auto offer = c.u32(c.u32(humans + table::records) + offset + field);
            if (offer == 0) return;
            c.put32(offer, 0);
            invoke(c, destroy_offer, back, {c.u32(c.u32(humans + table::records) + offset + field), 1});
        };
        for (; index < c.i32(humans + table::count); ++index, offset += record::size) {
            destroy_offer_at(record::first_offer, 0x10145f18);
            destroy_offer_at(record::second_offer, 0x10145f50);
            if (const auto entries = c.u32(c.u32(humans + table::records) + offset + record::entries); entries != 0)
                invoke(c, release_entries, 0x10145f6c, {entries});
            c.set({{31, std::uint32_t(index + 1)}, {28, offset + record::size}});
        }
        invoke(c, runtime::destroy_array, 0x10145f90, {c.u32(humans + table::records), c.toc(toc::record_destructor)});
        c.put32(humans + table::records, 0);
        c.put32(humans + table::count, 0);
    }
    if (std::int16_t(flag) > 0) invoke(c, release, 0x10145fac, {humans});
    return humans;
}

namespace {
// Copies the words of a template's tactic slot (`from` up to `to`, exclusive) to the human's slot.
void copy_words(Cpu& c, std::uint32_t slot, std::uint32_t tactic, std::uint32_t from, std::uint32_t to) {
    for (std::uint32_t at = from; at < to; at += 4) c.put32(slot + at, c.u32(tactic + at));
}
}

std::uint32_t start_human(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Frame frame(c, 0x10145fd0, 0x520, {.first_gpr = 25});
    const auto game_record = c.toc(toc::game), success = c.toc(toc::success), message = c.toc(toc::message),
               pool = c.toc(toc::pool), tactics_cell_address = c.toc(toc::tactics_templates);
    c.set({{25, humans}, {26, pool}, {27, message}, {28, game_record}, {29, staff}, {30, tactics_cell_address},
           {31, success}});
    // The contracts and screens objects are built in the frame and copied to the record at the end.
    const auto contracts = frame.at(0x98), screens = frame.at(0x48);
    invoke(c, initialize_contracts, 0x10146004, {contracts});
    invoke(c, initialize_screens, 0x1014600c, {screens});
    staff = staff_or_current(c, staff);
    c.set(29, staff);
    if (staff == 0) {
        report(c, frame, {0x10146040, 0x78, 0x40, 0xae});
        return 0;
    }
    // The index the original keeps in r28, in range or not.
    const auto raw = raw_index(c, staff);
    c.set(28, raw);
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x101460c0, 0x58, 0x38, 0xba});
        return 0;
    }
    const auto folders = {std::pair{text::tactics, 0x3fcu}, {text::search, 0x2fcu}, {text::training, 0x1fcu}, {text::print, 0xfcu}};
    std::uint32_t back = 0x10146110;
    for (const auto& [name, buffer] : folders) {
        invoke(c, folder_path, back, {pool + name, frame.at(std::int32_t(buffer))});
        back += 0xc;
    }
    const auto human = c.u32(humans + table::records) + *index * record::size;
    c.set({{27, 0}, {29, 0}, {26, human}});
    for (std::uint32_t slot_index = 0; std::int8_t(slot_index) < 5; ++slot_index) {
        const auto offset = c.r(29);
        const auto tactic = invoke(c, tactics_template, 0x10146164, {c.u32(tactics_cell_address), 0, 0});
        const auto slot = human + offset;
        c.set({{31, tactic}, {28, slot}});
        invoke(c, copy_bytes, 0x1014617c, {slot, tactic, 0x33});
        invoke(c, copy_bytes, 0x1014618c, {slot + 0x33, tactic + 0x33, 0x32});
        invoke(c, copy_bytes, 0x1014619c, {slot + 0x65, tactic + 0x65, 0xc8});
        copy_words(c, slot, tactic, 0x12e, 0x142);
        c.put16(slot + 0x142, c.u16(tactic + 0x142));
        invoke(c, copy_bytes, 0x101461dc, {slot + 0x144, tactic + 0x144, 0xb});
        copy_words(c, slot, tactic, 0x150, 0x164);
        c.put16(slot + 0x164, c.u16(tactic + 0x164));
        // Halfwords 0x166..0x586, two to a turn.
        for (std::uint32_t at = 0x166; at < 0x586; at += 4) {
            c.put16(slot + at, c.u16(tactic + at));
            c.put16(slot + at + 2, c.u16(tactic + at + 2));
        }
        copy_words(c, slot, tactic, 0x588, 0x5b4);
        c.put16(slot + 0x5b4, c.u16(tactic + 0x5b4));
        c.put16(slot + 0x5b6, c.u16(tactic + 0x5b6));
        invoke(c, copy_bytes, 0x101462b4, {slot + 0x5b8, tactic + 0x5b8, 0xb});
        copy_words(c, slot, tactic, 0x5c4, 0x5ec);
        c.put16(slot + 0x5ec, c.u16(tactic + 0x5ec));
        c.put16(slot + 0x5ee, c.u16(tactic + 0x5ee));
        // The original's pointer-update loops copy words 0x5f0..0xf34, 0xf38..0x187c, 0x1888..0x18dc: with the
        // word at 0xf34 (read past the first loop's end) and the three bytes after 0x1880.
        copy_words(c, slot, tactic, 0x5f0, 0xf38);
        copy_words(c, slot, tactic, 0xf38, 0x1880);
        c.put32(slot + 0x1880, c.u32(tactic + 0x1880));
        for (std::uint32_t at = 0x1884; at < 0x1887; ++at) c.put8(slot + at, c.u8(tactic + at));
        copy_words(c, slot, tactic, 0x1888, 0x18e0);
        c.put16(slot + tactic::field_18e0, 0);
        c.put16(slot + 0x18e2, 0);
        c.put16(slot + tactic::field_18e4, 0);
        c.put16(slot + 0x18e6, 0);
        c.put8(slot + tactic::field_18e8, 1);
        c.put8(slot + tactic::field_18e9, 1);
        c.put8(slot + tactic::field_18ea, 0);
        invoke(c, copy_text_bytes, 0x101463e0, {slot + 0x18eb, frame.at(0x3fc), 0x100});
        c.put8(slot + 0x19ea, 0);
        invoke(c, fill_bytes, 0x101463f8, {slot + 0x19eb, 0, 0x400});
        invoke(c, fill_bytes, 0x10146408, {slot + 0x1dec, 0xffffffffu, 0xc8});
        c.set({{29, offset + tactic_slot_size}, {27, slot_index + 1}});
    }
    if (c.i8(human + record::holiday) != 0) c.put32(humans + 8, c.u32(humans + 8) - 1);
    if (c.i8(human + record::away) != 0) c.put32(humans + 0xc, c.u32(humans + 0xc) - 1);
    for (const auto [field, back_to] : {std::pair{record::first_offer, 0x10146478u}, {record::second_offer, 0x1014649cu}}) {
        if (const auto offer = c.u32(human + field); offer != 0) {
            c.put32(offer, 0);
            invoke(c, destroy_offer, back_to, {c.u32(human + field), 1});
        }
    }
    c.put8(human + record::away, 0);
    c.put8(human + record::runs_club, 0);
    // The contracts object built in the frame, field by field (sizes as the original's loads and stores).
    const auto from_frame = [&](unsigned size, std::uint32_t at, std::int32_t local) {
        if (size == 4) c.put32(human + at, c.u32(frame.at(local)));
        else if (size == 2) c.put16(human + at, c.u16(frame.at(local)));
        else c.put8(human + at, c.u8(frame.at(local)));
    };
    for (std::uint32_t at = 0; at < 0x18; at += 4) from_frame(4, 0x998c + at, 0x98 + std::int32_t(at));
    from_frame(2, 0x99a4, 0xb0);
    from_frame(2, 0x99a6, 0xb2);
    invoke(c, copy_bytes, 0x101464fc, {human + 0x99a8, frame.at(0xb4), 5});
    struct Part { unsigned size; std::uint32_t at; std::int32_t local; };
    for (const auto& part : std::initializer_list<Part>{
             {4, 0x99b0, 0xbc}, {2, 0x99b6, 0xc2}, {2, 0x99b4, 0xc0}, {1, 0x99b8, 0xc4}, {2, 0x99be, 0xca},
             {2, 0x99bc, 0xc8}, {1, 0x99c0, 0xcc}, {1, 0x99c4, 0xd0}, {4, 0x99c8, 0xd4}, {1, 0x99cc, 0xd8},
             {2, 0x99ce, 0xda}, {1, 0x99d0, 0xdc}, {1, 0x99d1, 0xdd}, {1, 0x99d2, 0xde}, {4, 0x99d4, 0xe0},
             {1, 0x99d8, 0xe4}, {4, 0x99dc, 0xe8}, {1, 0x99e0, 0xec}, {4, 0x99e4, 0xf0}, {1, 0x99e8, 0xf4},
             {4, 0x99ec, 0xf8}})
        from_frame(part.size, part.at, part.local);
    c.put32(human + record::first_offer, 0);
    c.put32(human + record::second_offer, 0);
    c.put8(human + 0x99f8, 0);
    c.put32(human + record::setting, 0);
    c.put8(human + record::current_tactic, 0);
    c.put8(human + record::first_name, 0);
    c.put8(human + record::second_name, 0);
    c.put8(human + record::holiday, 0);
    c.put16(human + record::short_setting, 0x7d);
    invoke(c, fill_bytes, 0x101465e4, {human + 0x9a68, 0, 0x50});
    invoke(c, copy_text_bytes, 0x101465f8, {human + 0x9b40, frame.at(0x2fc), 0x100});
    c.put8(human + 0x9c3f, 0);
    invoke(c, fill_bytes, 0x10146614, {human + 0x9c40, 0, 0x400});
    invoke(c, copy_text_bytes, 0x10146628, {human + 0xa040, frame.at(0x1fc), 0x100});
    c.put8(human + 0xa13f, 0);
    invoke(c, fill_bytes, 0x10146644, {human + 0xa140, 0, 0x400});
    invoke(c, copy_text_bytes, 0x10146658, {human + 0xa540, frame.at(0xfc), 0x100});
    c.put8(human + 0xa63f, 0);
    for (std::uint32_t at = 0; at < 0xc; at += 4) from_frame(4, record::screens + at, 0x48 + std::int32_t(at));
    for (std::uint32_t at = 0xc; at < 0xf; ++at) from_frame(1, record::screens + at, 0x48 + std::int32_t(at));
    c.put32(human + record::entries, 0);
    c.put32(human + 0xa654, 0);
    c.put32(human + record::entry_count, 0);
    return 1;
}

std::uint32_t build_table(Cpu& c, std::uint32_t humans, std::uint32_t file) {
    const Frame frame(c, 0x10145af0, 0x390, {.first_gpr = 21});
    const auto staff_count_cell = c.toc(toc::staff_count), people = c.toc(toc::people), message = c.toc(toc::message),
               pool = c.toc(toc::pool), first_name = c.toc(first_cd_name), second_name = c.toc(second_cd_name);
    c.set({{23, humans}, {24, file}, {26, staff_count_cell}, {27, people}, {28, message}, {29, pool}, {30, first_name},
           {31, second_name}});
    // Wait for the game CD, as FINANCE.CPP's constructor does: both names are copied to the frame (16 and 17
    // bytes), merged one byte of each in turn until a zero byte of the second (whose zero lies past the copy,
    // so the loop ends on whatever the frame holds at +0x81), and looked up in the current folder.
    for (;;) {
        c.set(25, 0);
        const auto first_copy = frame.at(0x40), second_copy = frame.at(0x70), merged = frame.at(0xcc);
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(second_copy + at, c.u32(second_name + at));
        c.put8(second_copy + 0x10, c.u8(second_name + 0x10));
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(first_copy + at, c.u32(first_name + at));
        std::uint32_t walked = 0;
        for (; std::int8_t(c.u8(second_copy + 1 + walked)) != 0; ++walked) {
            c.put8(merged + 1 + 2 * walked, c.u8(second_copy + 1 + walked));
            c.put8(merged + 2 + 2 * walked, c.u8(first_copy + 1 + walked));
        }
        c.put8(merged, c.u8(second_copy) + c.u8(first_copy));
        c.set(21, first_copy + 1 + walked);
        // The words of the second name are still in r8-r10 and its length in r7 when the file system is asked.
        c.set({{10, c.u32(second_name)}, {9, c.u32(second_name + 4)}, {8, c.u32(second_name + 8)}, {7, c.u8(second_copy)}});
        const auto status = std::int16_t(invoke(c, make_file_spec, 0x10145bcc, {0, 0, merged, frame.at(0x84)}));
        c.set(2, c.u32(frame.at(0x14)));
        if (status == 0) c.set(25, 1);
        if (c.r(25) != 0) break;
        invoke(c, copy_text, 0x10145bf0, {frame.at(0x294), pool + insert_title});
        invoke(c, copy_text, 0x10145bfc, {frame.at(0x1cc), pool + insert_text});
        invoke(c, show_message, 0x10145c0c, {frame.at(0x294), frame.at(0x1cc), 1});
    }
    if (file != 0) {
        const auto read = invoke_unit(c, read_file, 0x10145c28, {humans, file});
        if (std::int8_t(read) != 0) return humans;
    }
    c.put32(humans + table::count, human_slots);
    c.set(21, 0);
    const auto count = c.u32(humans + table::count);
    c.set(24, count);
    auto array = invoke(c, runtime::allocate, 0x10145c5c, {count * record::size + 0x10});
    c.set(22, array);
    if (array != 0) {
        c.set(21, 1);
        array = invoke(c, runtime::construct_array, 0x10145c80,
                       {array, c.toc(toc::record_constructor), c.toc(toc::record_destructor), record::size, count});
        c.set(22, array);
    }
    c.put32(humans + table::records, array);
    if (c.u32(humans + table::records) == 0) {
        report(c, frame, {0x10145ca8, 0x50, 0x38, 0x4b});
        return humans;
    }
    invoke(c, fill_bytes, 0x10145d08, {c.u32(humans + table::records), 0, c.u32(humans + table::count) * record::size});
    c.set(22, 0);
    for (std::int32_t index = 0; index < c.i32(humans + table::count); ++index) {
        c.set(22, std::uint32_t(index));
        // The humans are the last 16 members of staff, taken from the end.
        const auto staff = c.u32(people) + (c.u32(staff_count_cell) - 1 - std::uint32_t(index)) * 0x70;
        c.set({{3, humans}, {4, staff}});
        c.link(0x10145d30);
        start_human(c, humans, staff);
        c.set(22, std::uint32_t(index) + 1);
    }
    c.put32(humans + 8, 0);
    c.put32(humans + 0xc, 0);
    return humans;
}
}
