// Build-time test oracle only. The shipped native executable never uses Ghidra
// or its emulator. Compare generated ARM64 routines against original bytes and
// independently calculated ABI results before advancing the port.
// @category CM0102
import ghidra.app.script.GhidraScript;
import ghidra.app.emulator.EmulatorHelper;
import ghidra.program.model.lang.Register;
import com.google.gson.*;
import java.math.BigInteger;
import java.nio.file.*;
import java.io.*;
import java.util.*;

public class ExportOracle extends GhidraScript {
    private static final long RETURN = 0xfffffff0L;
    private static final long STACK = 0x700fff00L;

    private String hex(byte[] bytes) { return HexFormat.of().formatHex(bytes); }

    @Override public void run() throws Exception {
        Path input = Path.of(getScriptArgs()[0]);
        Path output = Path.of(getScriptArgs()[1]);
        JsonObject spec = JsonParser.parseString(Files.readString(input)).getAsJsonObject();
        if (!currentProgram.getLanguageID().toString().equals("PowerPC:BE:32:default") ||
            !currentProgram.getExecutableSHA256().equals(spec.get("executableSHA256").getAsString()))
            throw new IllegalStateException("Oracle requires the exact original Mac executable");
        List<String> names = new ArrayList<>();
        for (int i = 0; i < 32; i++) { names.add("r" + i); names.add("f" + i); }
        for (int i = 0; i < 8; i++) names.add("cr" + i);
        names.addAll(List.of("LR", "CTR", "xer_so", "xer_ov", "xer_ca"));
        Path pending = Path.of(output + ".pending");
        int count = 0;
        try (BufferedWriter writer = Files.newBufferedWriter(pending)) {
            writer.write("SOURCE " + currentProgram.getExecutableSHA256() + "\n");
            for (JsonElement element : spec.getAsJsonArray("cases")) {
                monitor.checkCancelled();
                JsonObject item = element.getAsJsonObject();
                EmulatorHelper emu = new EmulatorHelper(currentProgram);
                try {
                    for (Register register : currentProgram.getLanguage().getRegisters())
                        if (!register.isProcessorContext()) emu.writeRegister(register, BigInteger.ZERO);
                    emu.writeMemory(toAddr(0x700f0000L), new byte[0x10000]);
                    emu.writeRegister("r1", STACK);
                    emu.writeRegister("r2", 0x108bba90L);
                    emu.writeRegister("LR", RETURN);
                    for (var entry : item.getAsJsonObject("initialRegisters").entrySet())
                        emu.writeRegister(entry.getKey(), new BigInteger(entry.getValue().getAsString(), 16));
                    long start = item.get("entry").getAsLong();
                    emu.writeRegister("pc", start);
                    List<String> initial = new ArrayList<>();
                    for (String name : names) {
                        Register reg = currentProgram.getRegister(name);
                        initial.add("I " + reg.getOffset() + " " + reg.getMinimumByteSize() + " " +
                            emu.readRegister(name).toString(16) + "\n");
                    }
                    long steps = 0;
                    while (emu.getExecutionAddress().getOffset() != RETURN) {
                        if (++steps > 10000) throw new IllegalStateException("Oracle instruction limit: " + item.get("name"));
                        if (!emu.step(monitor)) throw new IllegalStateException("Oracle failed: " + emu.getLastError());
                    }
                    for (var expected : item.getAsJsonObject("expectedABI").entrySet()) {
                        BigInteger actual = emu.readRegister(expected.getKey());
                        if (!actual.equals(new BigInteger(expected.getValue().getAsString(), 16)))
                            throw new IllegalStateException("Independent ABI expectation failed in " +
                                item.get("name") + " " + expected.getKey() + ": got " + actual.toString(16) +
                                " expected " + expected.getValue().getAsString());
                    }
                    writer.write("CASE " + item.get("name").getAsString() + " " + start + " " + steps + "\n");
                    for (String line : initial) writer.write(line);
                    for (String name : names) {
                        Register reg = currentProgram.getRegister(name);
                        writer.write("E " + reg.getOffset() + " " + reg.getMinimumByteSize() + " " +
                            emu.readRegister(name).toString(16) + "\n");
                    }
                    if (item.get("checkStack").getAsBoolean())
                        writer.write("M " + (STACK - 128) + " " + hex(emu.readMemory(toAddr(STACK - 128), 256)) + "\n");
                    writer.write("END\n");
                    count++;
                } finally { emu.dispose(); }
            }
        }
        Files.move(pending, output, StandardCopyOption.REPLACE_EXISTING);
        println("Exported " + count + " original PowerPC oracle cases with independent ABI expectations");
    }
}
