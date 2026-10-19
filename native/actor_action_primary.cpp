#include "actor_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_action {
using Call = SemanticGuestCall;
namespace {
bool score_cell_pressure(Call&,State&);
void primary_phase_1(Call&,State&);
void primary_phase_2(Call&,State&);
void primary_phase_3(Call&,State&);
void primary_phase_4(Call&,State&);
void primary_phase_5(Call&,State&);
void primary_phase_6(Call&,State&);
void primary_phase_7(Call&,State&);
void primary_phase_8(Call&,State&);
void primary_phase_9(Call&,State&);
void primary_phase_10(Call&,State&);
void primary_phase_11(Call&,State&);
void primary_phase_12(Call&,State&);
void primary_phase_13(Call&,State&);
void primary_phase_14(Call&,State&);
void primary_phase_15(Call&,State&);
void primary_phase_16(Call&,State&);
void primary_phase_17(Call&,State&);
void primary_phase_18(Call&,State&);
void primary_phase_19(Call&,State&);
void primary_phase_20(Call&,State&);
void primary_phase_21(Call&,State&);
void primary_phase_22(Call&,State&);
void primary_phase_23(Call&,State&);
void primary_phase_24(Call&,State&);
void primary_phase_25(Call&,State&);
void primary_phase_26(Call&,State&);
void primary_phase_27(Call&,State&);
void primary_phase_28(Call&,State&);
void primary_phase_29(Call&,State&);
void primary_phase_30(Call&,State&);
void primary_phase_31(Call&,State&);
void primary_phase_32(Call&,State&);
void primary_phase_33(Call&,State&);
void primary_phase_34(Call&,State&);
void primary_phase_35(Call&,State&);
void primary_phase_36(Call&,State&);
void primary_phase_37(Call&,State&);
void primary_phase_38(Call&,State&);
void primary_phase_39(Call&,State&);
void primary_phase_40(Call&,State&);
void primary_phase_41(Call&,State&);
void primary_phase_42(Call&,State&);
void primary_phase_43(Call&,State&);
void primary_phase_44(Call&,State&);
void primary_phase_45(Call&,State&);
void primary_phase_46(Call&,State&);
void primary_phase_47(Call&,State&);
void primary_phase_48(Call&,State&);
void primary_phase_49(Call&,State&);
void primary_phase_50(Call&,State&);
void primary_phase_51(Call&,State&);
void primary_phase_52(Call&,State&);
void primary_phase_53(Call&,State&);
void primary_phase_54(Call&,State&);
void primary_phase_55(Call&,State&);
void primary_phase_56(Call&,State&);
void primary_phase_57(Call&,State&);
void primary_phase_58(Call&,State&);
void primary_phase_59(Call&,State&);
void primary_phase_60(Call&,State&);
void primary_phase_61(Call&,State&);
void primary_phase_62(Call&,State&);
void primary_phase_63(Call&,State&);
void primary_phase_64(Call&,State&);
void primary_phase_65(Call&,State&);
void primary_phase_66(Call&,State&);
void primary_phase_67(Call&,State&);
void primary_phase_68(Call&,State&);
void primary_phase_69(Call&,State&);
void primary_phase_70(Call&,State&);
void primary_phase_71(Call&,State&);
void primary_phase_72(Call&,State&);
void primary_phase_73(Call&,State&);
void primary_phase_74(Call&,State&);
void primary_phase_75(Call&,State&);
void primary_phase_76(Call&,State&);
void primary_phase_77(Call&,State&);
void primary_phase_78(Call&,State&);
void primary_phase_79(Call&,State&);
void primary_phase_80(Call&,State&);
void primary_phase_81(Call&,State&);
void primary_phase_82(Call&,State&);
void primary_phase_83(Call&,State&);
void primary_phase_84(Call&,State&);
void primary_phase_85(Call&,State&);
void primary_phase_86(Call&,State&);
void primary_phase_87(Call&,State&);
void primary_phase_88(Call&,State&);
void primary_phase_89(Call&,State&);
void primary_phase_90(Call&,State&);
void primary_phase_91(Call&,State&);
void primary_phase_92(Call&,State&);
void primary_phase_93(Call&,State&);
void primary_phase_94(Call&,State&);
void primary_phase_95(Call&,State&);
void primary_phase_96(Call&,State&);
void primary_phase_97(Call&,State&);
void primary_phase_98(Call&,State&);
void primary_phase_99(Call&,State&);
void primary_phase_100(Call&,State&);
void primary_phase_101(Call&,State&);
void primary_phase_102(Call&,State&);
void primary_phase_103(Call&,State&);
void primary_phase_104(Call&,State&);
void primary_phase_105(Call&,State&);
void primary_phase_106(Call&,State&);
void primary_phase_107(Call&,State&);
void primary_phase_108(Call&,State&);
void primary_phase_109(Call&,State&);
void primary_phase_110(Call&,State&);
void primary_phase_111(Call&,State&);
void primary_phase_112(Call&,State&);
void primary_phase_113(Call&,State&);
void primary_phase_114(Call&,State&);
void primary_phase_115(Call&,State&);
void primary_phase_116(Call&,State&);
void primary_phase_117(Call&,State&);
void primary_phase_118(Call&,State&);
void primary_phase_119(Call&,State&);
void primary_phase_120(Call&,State&);
void primary_phase_121(Call&,State&);
void primary_phase_122(Call&,State&);
void primary_phase_123(Call&,State&);
void primary_phase_124(Call&,State&);
void primary_phase_125(Call&,State&);
void primary_phase_126(Call&,State&);
void primary_phase_127(Call&,State&);
void primary_phase_128(Call&,State&);
void primary_phase_129(Call&,State&);
void primary_phase_130(Call&,State&);
void primary_phase_131(Call&,State&);
void primary_phase_132(Call&,State&);
void primary_phase_133(Call&,State&);
void primary_phase_134(Call&,State&);
void primary_phase_135(Call&,State&);
void primary_phase_136(Call&,State&);
void primary_phase_137(Call&,State&);
void primary_phase_138(Call&,State&);
void primary_phase_139(Call&,State&);
void primary_phase_140(Call&,State&);
void primary_phase_141(Call&,State&);
void primary_phase_142(Call&,State&);
void primary_phase_143(Call&,State&);
void primary_phase_144(Call&,State&);
void primary_phase_145(Call&,State&);
void primary_phase_146(Call&,State&);
void primary_phase_147(Call&,State&);
void primary_phase_148(Call&,State&);
void primary_phase_149(Call&,State&);
void primary_phase_150(Call&,State&);
void primary_phase_151(Call&,State&);
void primary_phase_152(Call&,State&);
void primary_phase_153(Call&,State&);
void primary_phase_154(Call&,State&);
void primary_phase_155(Call&,State&);
void primary_phase_156(Call&,State&);
void primary_phase_157(Call&,State&);
void primary_phase_158(Call&,State&);
void primary_phase_159(Call&,State&);
void primary_phase_160(Call&,State&);
void primary_phase_161(Call&,State&);
void primary_phase_162(Call&,State&);
void primary_phase_163(Call&,State&);
void primary_phase_164(Call&,State&);
void primary_phase_165(Call&,State&);
void primary_phase_166(Call&,State&);
void primary_phase_167(Call&,State&);
void primary_phase_168(Call&,State&);
void primary_phase_169(Call&,State&);
void primary_phase_170(Call&,State&);
void primary_phase_171(Call&,State&);
void primary_phase_172(Call&,State&);
void primary_phase_173(Call&,State&);
void primary_phase_174(Call&,State&);
void primary_phase_175(Call&,State&);
void primary_phase_176(Call&,State&);
void primary_phase_177(Call&,State&);
void primary_phase_178(Call&,State&);
void primary_phase_179(Call&,State&);
void primary_phase_180(Call&,State&);
void primary_phase_181(Call&,State&);
void primary_phase_182(Call&,State&);
void primary_phase_183(Call&,State&);
void primary_phase_184(Call&,State&);
void primary_phase_185(Call&,State&);
void primary_phase_186(Call&,State&);
void primary_phase_187(Call&,State&);
void primary_phase_188(Call&,State&);
void primary_phase_189(Call&,State&);
void primary_phase_190(Call&,State&);
void primary_phase_191(Call&,State&);
void primary_phase_192(Call&,State&);
void primary_phase_193(Call&,State&);
void primary_phase_194(Call&,State&);
void primary_phase_195(Call&,State&);
void primary_phase_196(Call&,State&);
void primary_phase_197(Call&,State&);
void primary_phase_198(Call&,State&);
void primary_phase_199(Call&,State&);
void primary_phase_200(Call&,State&);
void primary_phase_201(Call&,State&);
void primary_phase_202(Call&,State&);
void primary_phase_203(Call&,State&);
void primary_phase_204(Call&,State&);
void primary_phase_205(Call&,State&);
void primary_phase_206(Call&,State&);
void primary_phase_207(Call&,State&);
void primary_phase_208(Call&,State&);
void primary_phase_209(Call&,State&);
void primary_phase_210(Call&,State&);
void primary_phase_211(Call&,State&);
void primary_phase_212(Call&,State&);
void primary_phase_213(Call&,State&);
void primary_phase_214(Call&,State&);
void primary_phase_215(Call&,State&);
void primary_phase_216(Call&,State&);
void primary_phase_217(Call&,State&);
void primary_phase_218(Call&,State&);
void primary_phase_219(Call&,State&);
void primary_phase_220(Call&,State&);
void primary_phase_221(Call&,State&);
void primary_phase_222(Call&,State&);
void primary_phase_223(Call&,State&);
void primary_phase_224(Call&,State&);
void primary_phase_225(Call&,State&);
void primary_phase_226(Call&,State&);
void primary_phase_227(Call&,State&);
void primary_phase_228(Call&,State&);
void primary_phase_229(Call&,State&);
void primary_phase_230(Call&,State&);
void primary_phase_231(Call&,State&);
void primary_phase_232(Call&,State&);
void primary_phase_233(Call&,State&);
void primary_phase_234(Call&,State&);
void primary_phase_235(Call&,State&);
void primary_phase_236(Call&,State&);
void primary_phase_237(Call&,State&);
void primary_phase_238(Call&,State&);
void primary_phase_239(Call&,State&);
void primary_phase_240(Call&,State&);
void primary_phase_241(Call&,State&);
void primary_phase_242(Call&,State&);
void primary_phase_243(Call&,State&);
void primary_phase_244(Call&,State&);
void primary_phase_245(Call&,State&);
void primary_phase_246(Call&,State&);
void primary_phase_247(Call&,State&);
void primary_phase_248(Call&,State&);
void primary_phase_249(Call&,State&);
void primary_phase_250(Call&,State&);
void primary_phase_251(Call&,State&);
void primary_phase_252(Call&,State&);
void primary_phase_253(Call&,State&);
void primary_phase_254(Call&,State&);
void primary_phase_255(Call&,State&);
void primary_phase_256(Call&,State&);
void primary_phase_257(Call&,State&);
void primary_phase_258(Call&,State&);
void primary_phase_259(Call&,State&);
void primary_phase_260(Call&,State&);
void primary_phase_261(Call&,State&);
void primary_phase_262(Call&,State&);
void primary_phase_263(Call&,State&);
void primary_phase_264(Call&,State&);
void primary_phase_265(Call&,State&);
void primary_phase_266(Call&,State&);
void primary_phase_267(Call&,State&);
void primary_phase_268(Call&,State&);
void primary_phase_269(Call&,State&);
void primary_phase_270(Call&,State&);
void primary_phase_271(Call&,State&);
void primary_phase_272(Call&,State&);
void primary_phase_273(Call&,State&);
void primary_phase_274(Call&,State&);
void primary_phase_275(Call&,State&);
void primary_phase_276(Call&,State&);
void primary_phase_277(Call&,State&);
void primary_phase_278(Call&,State&);
void primary_phase_279(Call&,State&);
void primary_phase_280(Call&,State&);
void primary_phase_281(Call&,State&);
void primary_phase_282(Call&,State&);
void primary_phase_283(Call&,State&);
void primary_phase_284(Call&,State&);
void primary_phase_285(Call&,State&);
void primary_phase_286(Call&,State&);
void primary_phase_287(Call&,State&);
void primary_phase_288(Call&,State&);
void primary_phase_289(Call&,State&);
void primary_phase_290(Call&,State&);
void primary_phase_291(Call&,State&);
void primary_phase_292(Call&,State&);
void primary_phase_293(Call&,State&);
void primary_phase_294(Call&,State&);
void primary_phase_295(Call&,State&);
void primary_phase_296(Call&,State&);
void primary_phase_297(Call&,State&);
void primary_phase_298(Call&,State&);
void primary_phase_299(Call&,State&);
void primary_phase_300(Call&,State&);
void primary_phase_301(Call&,State&);
void primary_phase_302(Call&,State&);
void primary_phase_303(Call&,State&);
void primary_phase_304(Call&,State&);
void primary_phase_305(Call&,State&);
void primary_phase_306(Call&,State&);
void primary_phase_307(Call&,State&);
void primary_phase_308(Call&,State&);
void primary_phase_309(Call&,State&);
void primary_phase_310(Call&,State&);
void primary_phase_311(Call&,State&);
void primary_phase_312(Call&,State&);
void primary_phase_313(Call&,State&);
void primary_phase_314(Call&,State&);
void primary_phase_315(Call&,State&);
void primary_phase_316(Call&,State&);
void primary_phase_317(Call&,State&);
void primary_phase_318(Call&,State&);
void primary_phase_319(Call&,State&);
void primary_phase_320(Call&,State&);
void primary_phase_321(Call&,State&);
void primary_phase_322(Call&,State&);
void primary_phase_323(Call&,State&);
void primary_phase_324(Call&,State&);
void primary_phase_325(Call&,State&);
void primary_phase_326(Call&,State&);
void primary_phase_327(Call&,State&);
void primary_phase_328(Call&,State&);
void primary_phase_329(Call&,State&);
void primary_phase_330(Call&,State&);
void primary_phase_331(Call&,State&);
void primary_phase_332(Call&,State&);
void primary_phase_333(Call&,State&);
void primary_phase_334(Call&,State&);
void primary_phase_335(Call&,State&);
void primary_phase_336(Call&,State&);
void primary_phase_337(Call&,State&);
void primary_phase_338(Call&,State&);
void primary_phase_339(Call&,State&);
void primary_phase_340(Call&,State&);
void primary_phase_341(Call&,State&);
void primary_phase_342(Call&,State&);
void primary_phase_343(Call&,State&);
void primary_phase_344(Call&,State&);
void primary_phase_345(Call&,State&);
void primary_phase_346(Call&,State&);
void primary_phase_347(Call&,State&);
void primary_phase_348(Call&,State&);
void primary_phase_349(Call&,State&);
void primary_phase_350(Call&,State&);
void primary_phase_351(Call&,State&);
void primary_phase_352(Call&,State&);
void primary_phase_353(Call&,State&);
void primary_phase_354(Call&,State&);
void primary_phase_355(Call&,State&);
void primary_phase_356(Call&,State&);
void primary_phase_357(Call&,State&);
void primary_phase_358(Call&,State&);
void primary_phase_359(Call&,State&);
void primary_phase_360(Call&,State&);
void primary_phase_361(Call&,State&);
void primary_phase_362(Call&,State&);
void primary_phase_363(Call&,State&);
void primary_phase_364(Call&,State&);
void primary_phase_365(Call&,State&);
void primary_phase_366(Call&,State&);
void primary_phase_367(Call&,State&);
void primary_phase_368(Call&,State&);
void primary_phase_369(Call&,State&);
void primary_phase_370(Call&,State&);
void primary_phase_371(Call&,State&);
void primary_phase_372(Call&,State&);
void primary_phase_373(Call&,State&);
void primary_phase_374(Call&,State&);
void primary_phase_375(Call&,State&);
void primary_phase_376(Call&,State&);
void primary_phase_377(Call&,State&);
void primary_phase_378(Call&,State&);
void primary_phase_379(Call&,State&);
void primary_phase_380(Call&,State&);
void primary_phase_381(Call&,State&);
void primary_phase_382(Call&,State&);
void primary_phase_383(Call&,State&);
void primary_phase_384(Call&,State&);
void primary_phase_385(Call&,State&);
void primary_phase_386(Call&,State&);
void primary_phase_387(Call&,State&);
void primary_phase_388(Call&,State&);
void primary_phase_389(Call&,State&);
void primary_phase_390(Call&,State&);
void primary_phase_391(Call&,State&);
void primary_phase_392(Call&,State&);
void primary_phase_393(Call&,State&);
void primary_phase_394(Call&,State&);
void primary_phase_395(Call&,State&);
void primary_phase_396(Call&,State&);
void primary_phase_397(Call&,State&);
void primary_phase_398(Call&,State&);
void primary_phase_399(Call&,State&);
void primary_phase_400(Call&,State&);
void primary_phase_401(Call&,State&);
void primary_phase_402(Call&,State&);
void primary_phase_403(Call&,State&);
void primary_phase_404(Call&,State&);
void primary_phase_405(Call&,State&);
void primary_phase_406(Call&,State&);
void primary_phase_407(Call&,State&);
void primary_phase_408(Call&,State&);
void primary_phase_409(Call&,State&);
void primary_phase_410(Call&,State&);
void primary_phase_411(Call&,State&);
void primary_phase_412(Call&,State&);
void primary_phase_413(Call&,State&);
void primary_phase_414(Call&,State&);
void primary_phase_415(Call&,State&);
void primary_phase_416(Call&,State&);
void primary_phase_417(Call&,State&);
void primary_phase_418(Call&,State&);
void primary_phase_419(Call&,State&);
void primary_phase_420(Call&,State&);
void primary_phase_421(Call&,State&);
void primary_phase_422(Call&,State&);
void primary_phase_423(Call&,State&);
void primary_phase_424(Call&,State&);
void primary_phase_425(Call&,State&);
void primary_phase_426(Call&,State&);
void primary_phase_427(Call&,State&);
void primary_phase_428(Call&,State&);
void primary_phase_429(Call&,State&);
void primary_phase_430(Call&,State&);
void primary_phase_431(Call&,State&);
void primary_phase_432(Call&,State&);
void primary_phase_433(Call&,State&);
void primary_phase_434(Call&,State&);
void primary_phase_435(Call&,State&);
void primary_phase_436(Call&,State&);
void primary_phase_437(Call&,State&);
void primary_phase_438(Call&,State&);
void primary_phase_439(Call&,State&);
void primary_phase_440(Call&,State&);
void primary_phase_441(Call&,State&);
void primary_phase_442(Call&,State&);
void primary_phase_443(Call&,State&);
void primary_phase_444(Call&,State&);
void primary_phase_445(Call&,State&);
void primary_phase_446(Call&,State&);
void primary_phase_447(Call&,State&);
void primary_phase_448(Call&,State&);
void primary_phase_449(Call&,State&);
void primary_phase_450(Call&,State&);
void primary_phase_451(Call&,State&);
void primary_phase_452(Call&,State&);
void primary_phase_453(Call&,State&);
void primary_phase_454(Call&,State&);
void primary_phase_455(Call&,State&);
void primary_phase_456(Call&,State&);
void primary_phase_457(Call&,State&);
void primary_phase_458(Call&,State&);
void primary_phase_459(Call&,State&);
void primary_phase_460(Call&,State&);
void primary_phase_461(Call&,State&);
void primary_phase_462(Call&,State&);
void primary_phase_463(Call&,State&);
void primary_phase_464(Call&,State&);
void primary_phase_465(Call&,State&);
void primary_phase_466(Call&,State&);
void primary_phase_467(Call&,State&);
void primary_phase_468(Call&,State&);
void primary_phase_469(Call&,State&);
void primary_phase_470(Call&,State&);
void primary_phase_471(Call&,State&);
void primary_phase_472(Call&,State&);
void primary_phase_473(Call&,State&);
void primary_phase_474(Call&,State&);
void primary_phase_475(Call&,State&);
void primary_phase_476(Call&,State&);
void primary_phase_477(Call&,State&);
void primary_phase_478(Call&,State&);
void primary_phase_479(Call&,State&);
void primary_phase_480(Call&,State&);
void primary_phase_481(Call&,State&);
void primary_phase_482(Call&,State&);
void primary_phase_483(Call&,State&);
void primary_phase_484(Call&,State&);
void primary_phase_485(Call&,State&);
void primary_phase_486(Call&,State&);
void primary_phase_487(Call&,State&);
void primary_phase_488(Call&,State&);
void primary_phase_489(Call&,State&);
void primary_phase_490(Call&,State&);
void primary_phase_491(Call&,State&);
void primary_phase_492(Call&,State&);
void primary_phase_493(Call&,State&);
void primary_phase_494(Call&,State&);
void primary_phase_495(Call&,State&);
void primary_phase_496(Call&,State&);
void primary_phase_497(Call&,State&);
void primary_phase_498(Call&,State&);
void primary_phase_499(Call&,State&);
void primary_phase_500(Call&,State&);
void primary_phase_501(Call&,State&);
void primary_phase_502(Call&,State&);
void primary_phase_503(Call&,State&);
void primary_phase_504(Call&,State&);
void primary_phase_505(Call&,State&);
void primary_phase_506(Call&,State&);
void primary_phase_507(Call&,State&);
void primary_phase_508(Call&,State&);
bool score_cell_pressure(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x1027f730); a.load<1>(0,15,0x27); a.sign8(4,23); a.sign8(0,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.add(3,17,0); a.load<1>(0,3,0x58); a.sign8(0,0);
        a.compare_registers(4,0);
        if (a.at_least()) return true;
        a.jump(0x1027f548); a.at(0x1027f548); a.load<1>(3,15,0x27); a.sign8(0,23); a.mask(0,0,0xfffffffc,false,2);
        a.sign8(3,3); a.leading_zeroes(3,3); a.mask(3,3,0x7ffffff,false,27); a.sign8(3,3); a.multiply(3,3,0x2c);
        a.add(0,3,0); a.indexed_load(3,17,0); a.load<1>(0,3,0x24); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027f57c); a.load<1>(3,15,0x17); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
            a.floating_load(1,26,128); a.multiply(0,0,0x64); a.floating_load(3,31,244,true);
            a.floating_load(2,15,152,true); a.exclusive_flags(0,0,0x80000000); a.store(1,0xa8,0);
            a.floating_load(0,1,164); a.floating_subtract(0,0,1,true); a.floating_product_sum(0,3,2,0,true);
            a.floating_integer(0,0); a.floating_store(0,1,172); a.load(14,1,0xb0); a.compare(14,0);
            if (!a.equal()) {
                a.at(0x1027f5cc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027f5e4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027f5f4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
                if (a.positive()) {
                    a.at(0x1027f614); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
                    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0);
                    a.value(5,0x10000); a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164);
                    a.offset(6,6,-0x7fff); a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2);
                    a.load<2>(8,7,0x0); a.load(7,27,0x0); a.multiply(0,14,0xa); a.offset(3,3,-0x7fff);
                    a.store(1,0xac,4); a.floating_difference(2,1,0,true); a.add(7,8,7); a.multiply_high(4,6,7);
                    a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1);
                    a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7); a.exclusive_flags(4,13,0x80000000);
                    a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
                    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
                    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180);
                    a.load(13,1,0xb8); a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5);
                    a.offset(4,4,0x6667); a.offset(0,14,-0x1); a.multiply_high(3,4,3); a.shift_right(3,3,0x2);
                    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3); a.compare_registers(13,0);
                    if (a.nonpositive()) {
                    } else {
                        a.at(0x1027f6f0); a.offset(13,14,-0x1); a.jump(0x1027f728);
                    }
                } else {
                    a.at(0x1027f604); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
                    if (a.at_least()) {
                        a.at(0x1027f6f8); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
                        a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
                        if (a.positive()) {
                            a.at(0x1027f720); a.move(0,13);
                        }
                        a.at(0x1027f724); a.move(13,0);
                    } else {
                        a.at(0x1027f614); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000);
                        a.store(1,0xa8,3); a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000);
                        a.store(1,0xa4,0); a.value(5,0x10000); a.floating_load(2,26,128); a.value(3,0x80010000);
                        a.floating_load(1,1,164); a.offset(6,6,-0x7fff); a.floating_load(0,26,0);
                        a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
                        a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4);
                        a.floating_difference(2,1,0,true); a.add(7,8,7); a.multiply_high(4,6,7);
                        a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1);
                        a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7); a.exclusive_flags(4,13,0x80000000);
                        a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
                        a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf);
                        a.floating_product(0,2,0); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0);
                        a.floating_store(0,1,180); a.load(13,1,0xb8); a.call(0x10226520); a.value(4,0x66660000);
                        a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1); a.multiply_high(3,4,3);
                        a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
                        a.compare_registers(13,0);
                        if (a.nonpositive()) {
                        } else {
                            a.at(0x1027f6f0); a.offset(13,14,-0x1); a.jump(0x1027f728);
                        }
                    }
                }
            } else {
                a.at(0x1027f5c4); a.value(13,0x0); a.jump(0x1027f728);
            }
            a.at(0x1027f728); a.add(19,19,13);
        }
        a.at(0x1027f72c); a.offset(23,23,0x1);
        continue;
    }
}
void primary_phase_508(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281748); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13);
}
void primary_phase_507(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102816e0); a.load(3,2,-0x51e4); a.offset(6,1,0x84); a.offset(7,1,0x6c); a.value(4,0x0);
    a.offset(3,3,0x8f); a.value(5,0x0); a.call(0x10739890); a.call(0x10768810); a.load(4,2,-0x51e4); a.move(5,3);
    a.load(3,2,-0x7ba8); a.offset(6,1,0x84); a.offset(4,4,0x9c); a.offset(7,1,0x6c); a.value(8,0x50af);
    a.call(0x1077b720); a.load(3,2,-0x51e4); a.value(5,0x0); a.load(4,2,-0x7ba8); a.offset(3,3,0xa9);
    a.call(0x10769b60); a.load(4,2,-0x7bac); a.value(0,0x0); a.value(3,0xffffffff); a.store<1>(4,0x0,0);
    a.call(0x1076f7d0);
    primary_phase_508(a,state);
}
void primary_phase_506(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102816ac); a.load<2>(13,1,0x74,true); a.move(21,0); a.load(3,15,0x1a4); a.call(0x101d69c0);
    a.load<1>(4,1,0x51); a.move(7,3); a.load<2>(6,1,0x5c,true); a.move(3,15); a.sign16(5,21); a.sign8(8,13);
    a.offset(9,1,0x74); a.call(0x102576e0); a.jump(0x10281748);
    primary_phase_508(a,state);
}
void primary_phase_505(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281650); a.load<1>(0,1,0x101); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10281698); a.move(3,15); a.offset(4,1,0x51); a.offset(5,1,0x5c); a.call(0x10252080); a.sign16(0,3);
    } else {
        a.at(0x1028165c); a.move(3,15); a.offset(4,1,0x51); a.offset(5,1,0x5c); a.call(0x10252080);
        a.load(0,1,0x104); a.sign16(5,3); a.mask(3,5,0x1,false,1); a.mask(4,0,0x1,false,1); a.load(0,1,0x104);
        a.add(3,3,5); a.add(0,4,0); a.shift_right(3,3,0x1); a.shift_right(0,0,0x1); a.add(0,0,3);
        a.jump(0x102816ac);
    }
    a.at(0x102816ac);
    primary_phase_506(a,state);
}
void primary_phase_504(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028158c); a.load<1>(4,1,0x55); a.move(3,15); a.load<1>(5,1,0x54); a.move(8,16); a.load<1>(7,1,0xff);
    a.value(6,0x12); a.call(0x1022e7e0); a.value(0,0x12); a.store<2>(15,0x19c,0); a.jump(0x10281748);
    primary_phase_508(a,state);
}
void primary_phase_503(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281564); a.load<1>(4,1,0x55); a.move(3,15); a.load<1>(5,1,0x54); a.move(8,16); a.load<1>(7,1,0xff);
    a.value(6,0x11); a.call(0x1022e7e0); a.value(0,0x11); a.store<2>(15,0x19c,0); a.jump(0x10281748);
    primary_phase_508(a,state);
}
void primary_phase_502(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028147c); a.load<2>(0,15,0x19c,true); a.compare(0,18);
    if (a.equal()) {
        a.at(0x1028158c);
        primary_phase_504(a,state);
    } else {
        a.at(0x10281488); a.load<2>(0,15,0x19c,true); a.compare(0,17);
        if (a.equal()) {
            a.at(0x1028158c);
            primary_phase_504(a,state);
        } else {
            a.at(0x10281494); a.load<1>(0,1,0xff); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x102814a0); a.load(3,15,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x1fd8);
                a.compare_registers(15,0,false);
                if (!a.equal()) {
                } else {
                    a.at(0x102814b4); a.load(3,15,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x1fdc);
                    a.compare(0,0,false);
                    if (a.equal()) {
                    } else {
                        a.at(0x102814c8); a.load<2>(0,15,0x2a,true); a.compare(0,-100);
                        if (a.nonpositive()) {
                        } else {
                            a.at(0x102814d4); a.load(3,15,0x1a4); a.offset(3,3,0x10000); a.load(3,3,-0x1fdc);
                            a.load<2>(0,3,0x19c,true); a.compare(0,52);
                            if (a.equal()) {
                            } else {
                                a.at(0x102814ec); a.load(3,15,0x1a4); a.offset(3,3,0x10000); a.load(3,3,-0x1fdc);
                                a.load<2>(0,3,0x19c,true); a.compare(0,55);
                                if (a.equal()) {
                                } else {
                                    a.at(0x10281504); a.load(4,15,0x1a4); a.value(3,0x55550000);
                                    a.offset(3,3,0x5556); a.offset(4,4,0x10000); a.load(5,4,-0x1fdc);
                                    a.load<1>(4,5,0x3); a.offset(0,4,0x1); a.store<1>(5,0x3,0);
                                    a.load<2>(0,15,0x2a,true); a.load(4,15,0x1a4); a.multiply_high(3,3,0);
                                    a.offset(4,4,0x10000); a.load(5,4,-0x1fdc); a.load<2>(4,5,0x36,true);
                                    a.mask(0,3,0x1,false,1); a.add(3,3,0); a.offset(0,3,0x113); a.sign16(0,0);
                                    a.add(0,4,0); a.store<2>(5,0x36,0); a.load(4,15,0x1a4); a.load(3,4,0x3108);
                                    a.offset(0,3,0x113); a.store(4,0x3108,0);
                                }
                            }
                        }
                    }
                }
            }
            a.at(0x10281564);
            primary_phase_503(a,state);
        }
    }
}
void primary_phase_501(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281438); a.load(3,15,0x1a4); a.load<1>(0,3,0x1c0); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10281448); a.load(3,15,0x1a4); a.value(4,0x1f44); a.load<1>(7,15,0x27); a.value(8,0xffffffff);
        a.load(3,3,0x79f4); a.value(9,0xffffffff); a.load<1>(6,15,0x2f); a.value(10,0x0); a.load(5,15,0x1a4);
        a.call(0x10201180); a.load<2>(3,15,0x36,true); a.offset(0,3,0x19); a.store<2>(15,0x36,0);
    }
    a.at(0x1028147c);
    primary_phase_502(a,state);
}
void primary_phase_500(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281420); a.load<1>(4,15,0x105); a.move(3,15); a.load<1>(5,15,0x106); a.load<1>(6,1,0x55);
    a.load<1>(7,1,0x54); a.call(0x10237830);
    primary_phase_501(a,state);
}
void primary_phase_499(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102813b8); a.load<1>(5,1,0x55); a.value(0,0x6); a.load<1>(4,15,0x27); a.move(3,15); a.sign8(6,5);
    a.load<1>(5,1,0x54); a.offset(7,6,0x4); a.sign8(4,4); a.mask(6,7,0x1,false,1); a.sign8(8,5);
    a.leading_zeroes(5,4); a.load<1>(4,15,0x105); a.add(9,6,7); a.mask(7,8,0x1,false,1); a.mask(6,5,0x1,false,27);
    a.load<1>(5,15,0x106); a.shift_right(9,9,0x1); a.add(8,7,8); a.negate(7,6); a.shift_right(8,8,0x1);
    a.sign8(6,9); a.intersect(0,0,7); a.add(0,8,0); a.sign8(7,0); a.call(0x10237830); a.jump(0x10281438);
    primary_phase_501(a,state);
}
void primary_phase_498(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281394); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10281420);
        primary_phase_500(a,state);
    } else {
        a.at(0x102813a0); a.load<1>(3,1,0x55); a.load<1>(0,15,0x105); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.positive()) {
            a.at(0x10281420);
            primary_phase_500(a,state);
        } else {
            a.at(0x102813b8);
            primary_phase_499(a,state);
        }
    }
}
void primary_phase_497(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028136c); a.store<1>(1,0x54,0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10281394);
        primary_phase_498(a,state);
    } else {
        a.at(0x1028137c); a.load<1>(3,1,0x55); a.load<1>(0,15,0x105); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x102813b8);
            primary_phase_499(a,state);
        } else {
            a.at(0x10281394);
            primary_phase_498(a,state);
        }
    }
}
void primary_phase_496(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281334); a.load<1>(0,1,0x4c); a.store<1>(1,0x55,3); a.sign8(0,0,true);
    if (a.at_least()) {
        a.at(0x1028134c); a.load<1>(0,1,0x4c); a.sign8(0,0); a.compare(0,11);
        if (a.nonpositive()) {
            a.at(0x10281364); a.load<1>(0,1,0x4c); a.sign8(0,0);
        } else {
            a.at(0x1028135c); a.value(0,0xb); a.jump(0x1028136c);
        }
    } else {
        a.at(0x10281344); a.value(0,0x0); a.jump(0x1028136c);
    }
    a.at(0x1028136c);
    primary_phase_497(a,state);
}
void primary_phase_495(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281300); a.load<1>(0,1,0x4d); a.sign8(0,0,true);
    if (a.at_least()) {
        a.at(0x10281314); a.load<1>(0,1,0x4d); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1028132c); a.load<1>(3,1,0x4d); a.sign8(3,3);
        } else {
            a.at(0x10281324); a.value(3,0x8); a.jump(0x10281334);
        }
    } else {
        a.at(0x1028130c); a.value(3,0x0); a.jump(0x10281334);
    }
    a.at(0x10281334);
    primary_phase_496(a,state);
}
void primary_phase_494(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102812c8); a.value(0,0x9); a.move(3,15); a.store<2>(15,0x19c,0); a.call(0x10285790); a.move(4,3);
    a.move(3,15); a.value(5,0x0); a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff);
    a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_493(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281244); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,27,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0xa); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x102812c8);
        primary_phase_494(a,state);
    } else {
        a.at(0x10281278); a.load<1>(5,15,0x27); a.value(0,0xb); a.load(3,15,0x1a4); a.move(4,15); a.sign8(7,5);
        a.value(5,0x4); a.negate(6,7); a.combine(6,6,7); a.shift_right(6,6,0x1f); a.intersect(0,0,6,true);
        a.sign8(6,0); a.call(0x101eeac0); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x102812c8);
            primary_phase_494(a,state);
        } else {
            a.at(0x102812b0); a.move(3,15); a.value(4,0x0); a.call(0x10274170); a.value(0,0x10);
            a.store<2>(15,0x19c,0); a.jump(0x10281748);
            primary_phase_508(a,state);
        }
    }
}
void primary_phase_492(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102811fc); a.sign8(0,18,true);
    if (!a.equal()) {
        a.at(0x10281300);
        primary_phase_495(a,state);
    } else {
        a.at(0x10281204); a.sign8(0,16); a.compare(0,4);
        if (a.equal()) {
            a.at(0x10281300);
            primary_phase_495(a,state);
        } else {
            a.at(0x10281210); a.load<1>(0,15,0x10c); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x10281300);
                primary_phase_495(a,state);
            } else {
                a.at(0x1028121c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x10281234); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x10281244);
                primary_phase_493(a,state);
            }
        }
    }
}
void primary_phase_491(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102811d4); a.load<1>(4,15,0x105); a.move(3,15); a.load<1>(5,15,0x106); a.value(6,0xf); a.value(7,0x0);
    a.value(8,0x0); a.call(0x1022e7e0); a.value(0,0xf); a.store<2>(15,0x19c,0); a.jump(0x10281748);
    primary_phase_508(a,state);
}
void primary_phase_490(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102811cc); a.sign8(7,0); a.call(0x10237830);
    primary_phase_491(a,state);
}
void primary_phase_489(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028118c); a.sign8(7,0); a.call(0x10237830); a.jump(0x102811d4);
    primary_phase_491(a,state);
}
void primary_phase_488(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028114c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10281198); a.load<1>(0,15,0x106); a.move(3,15); a.load<1>(4,15,0x105); a.value(6,0x4); a.sign8(0,0);
        a.load<1>(5,15,0x106); a.compare(0,3);
        if (a.nonpositive()) {
            a.at(0x102811c8); a.value(0,0x0);
        } else {
            a.at(0x102811b8); a.load<1>(0,15,0x106); a.sign8(7,0); a.offset(0,7,-0x3); a.jump(0x102811cc);
        }
        a.at(0x102811cc);
        primary_phase_490(a,state);
    } else {
        a.at(0x10281158); a.load<1>(0,15,0x106); a.move(3,15); a.load<1>(4,15,0x105); a.value(6,0x4); a.sign8(0,0);
        a.load<1>(5,15,0x106); a.compare(0,9);
        if (a.at_least()) {
            a.at(0x10281188); a.value(0,0xb);
        } else {
            a.at(0x10281178); a.load<1>(0,15,0x106); a.sign8(7,0); a.offset(0,7,0x3); a.jump(0x1028118c);
        }
        a.at(0x1028118c);
        primary_phase_489(a,state);
    }
}
void primary_phase_487(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281130); a.sign8(0,3,true);
    if (!a.equal()) {
        a.at(0x102811fc);
        primary_phase_492(a,state);
    } else {
        a.at(0x10281138); a.sign8(0,18,true);
        if (a.equal()) {
            a.at(0x1028114c);
            primary_phase_488(a,state);
        } else {
            a.at(0x10281140); a.sign8(0,16); a.compare(0,4);
            if (!a.equal()) {
                a.at(0x102811fc);
                primary_phase_492(a,state);
            } else {
                a.at(0x1028114c);
                primary_phase_488(a,state);
            }
        }
    }
}
void primary_phase_486(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281110); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0x0); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60);
    primary_phase_487(a,state);
}
void primary_phase_485(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102810b8); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0xb); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60); a.jump(0x10281130);
    primary_phase_487(a,state);
}
void primary_phase_484(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281078); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x102810dc); a.load<1>(0,15,0x106); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10281110);
            primary_phase_486(a,state);
        } else {
            a.at(0x102810e8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x10281110);
                primary_phase_486(a,state);
            } else {
                a.at(0x102810f8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                if (a.positive()) {
                    a.at(0x10281110);
                    primary_phase_486(a,state);
                } else {
                    a.at(0x10281108); a.value(3,0x1); a.jump(0x10281130);
                    primary_phase_487(a,state);
                }
            }
        }
    } else {
        a.at(0x10281084); a.load<1>(0,15,0x106); a.compare(0,11);
        if (!a.equal()) {
            a.at(0x102810b8);
            primary_phase_485(a,state);
        } else {
            a.at(0x10281090); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x102810b8);
                primary_phase_485(a,state);
            } else {
                a.at(0x102810a0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                if (a.positive()) {
                    a.at(0x102810b8);
                    primary_phase_485(a,state);
                } else {
                    a.at(0x102810b0); a.value(3,0x1); a.jump(0x10281130);
                    primary_phase_487(a,state);
                }
            }
        }
    }
}
void primary_phase_483(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281074); a.value(18,0x1);
    primary_phase_484(a,state);
}
void primary_phase_482(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281064); a.value(3,0x0); a.value(0,0x0); a.store<1>(1,0x4d,3); a.store<1>(1,0x4c,0);
    primary_phase_483(a,state);
}
void primary_phase_481(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281050); a.value(3,0x8); a.value(0,0x0); a.store<1>(1,0x4d,3); a.store<1>(1,0x4c,0);
    a.jump(0x10281074);
    primary_phase_483(a,state);
}
void primary_phase_480(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028102c); a.load(3,30,0x0); a.load(0,27,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (!a.equal()) {
        a.at(0x10281064);
        primary_phase_482(a,state);
    } else {
        a.at(0x10281050);
        primary_phase_481(a,state);
    }
}
void primary_phase_479(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280fd4); a.value(3,0x0); a.value(0,0xb); a.store<1>(1,0x4d,3); a.store<1>(1,0x4c,0);
    a.jump(0x10281074);
    primary_phase_483(a,state);
}
void primary_phase_478(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280fc0); a.value(3,0x8); a.value(0,0xb); a.store<1>(1,0x4d,3); a.store<1>(1,0x4c,0);
    a.jump(0x10281074);
    primary_phase_483(a,state);
}
void primary_phase_477(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280f9c); a.load(3,30,0x0); a.load(0,27,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (!a.equal()) {
        a.at(0x10280fd4);
        primary_phase_479(a,state);
    } else {
        a.at(0x10280fc0);
        primary_phase_478(a,state);
    }
}
void primary_phase_476(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280eac); a.sign8(0,14); a.compare(0,3);
    if (a.equal()) {
        a.at(0x102815cc); a.sign8(0,16); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x102815f0); a.load(3,15,0x1a4); a.load<1>(0,3,0x1c0); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x10281600); a.load<2>(0,15,0x19c,true); a.compare(0,59);
                if (a.equal()) {
                } else {
                    a.at(0x1028160c); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0,true);
                    if (!a.equal()) {
                    } else {
                        a.at(0x1028161c); a.load(3,15,0x1a4); a.value(4,0x1f44); a.load<1>(7,15,0x27);
                        a.value(8,0xffffffff); a.load(3,3,0x79f4); a.value(9,0xffffffff); a.load<1>(6,15,0x2f);
                        a.value(10,0x0); a.load(5,15,0x1a4); a.call(0x10201180); a.load<2>(3,15,0x36,true);
                        a.offset(0,3,0x19); a.store<2>(15,0x36,0);
                    }
                }
            }
            a.at(0x10281650);
            primary_phase_505(a,state);
        } else {
            a.at(0x102815d8); a.offset(3,1,0x74); a.value(13,0x1); a.value(4,0xffffffff); a.call(0x10738c20);
            a.move(3,13); a.jump(0x1028175c);
        }
    } else {
        a.at(0x10280eb8);
        if (a.at_least()) {
            a.at(0x10280ecc); a.compare(0,5);
            if (a.at_least()) {
                a.at(0x102816e0);
                primary_phase_507(a,state);
            } else {
                a.at(0x10280ed4); a.jump(0x10280f40); a.load<1>(0,1,0x102); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x10281078);
                    primary_phase_484(a,state);
                } else {
                    a.at(0x10280f4c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
                    if (!a.equal()) {
                        a.at(0x10280fe8); a.load<1>(0,1,0x53); a.sign8(0,0); a.compare(0,4);
                        if (a.positive()) {
                            a.at(0x10281050);
                            primary_phase_481(a,state);
                        } else {
                            a.at(0x10280ff8); a.load<1>(0,1,0x53); a.compare(0,4);
                            if (!a.equal()) {
                                a.at(0x10281064);
                                primary_phase_482(a,state);
                            } else {
                                a.at(0x10281004); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(30,0x0,3);
                                if (a.positive()) {
                                    a.at(0x1028101c); a.store(30,0x0,28); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
                                }
                                a.at(0x1028102c);
                                primary_phase_480(a,state);
                            }
                        }
                    } else {
                        a.at(0x10280f58); a.load<1>(0,1,0x53); a.sign8(0,0); a.compare(0,4);
                        if (a.positive()) {
                            a.at(0x10280fc0);
                            primary_phase_478(a,state);
                        } else {
                            a.at(0x10280f68); a.load<1>(0,1,0x53); a.compare(0,4);
                            if (!a.equal()) {
                                a.at(0x10280fd4);
                                primary_phase_479(a,state);
                            } else {
                                a.at(0x10280f74); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(30,0x0,3);
                                if (a.positive()) {
                                    a.at(0x10280f8c); a.store(30,0x0,28); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
                                }
                                a.at(0x10280f9c);
                                primary_phase_477(a,state);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x10280ebc); a.compare(0,1);
            if (a.equal()) {
                a.at(0x10280ed8); a.compare(22,0,false);
                if (!a.equal()) {
                    a.at(0x10280f18); a.value(0,0x6c); a.move(3,15); a.store<2>(15,0x19c,0); a.move(5,22);
                    a.load<1>(4,1,0x50); a.load<1>(6,1,0x56); a.load<1>(7,1,0x4f); a.load<1>(8,1,0x4e);
                    a.call(0x10245220); a.jump(0x10281748);
                    primary_phase_508(a,state);
                } else {
                    a.at(0x10280ee0); a.value(0,0x9); a.move(3,15); a.store<2>(15,0x19c,0); a.call(0x10285790);
                    a.move(4,3); a.move(3,15); a.value(5,0x0); a.call(0x10245870); a.offset(3,1,0x74);
                    a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
                }
            } else {
                a.at(0x10280ec4);
                if (a.at_least()) {
                    a.at(0x102815b4); a.move(3,15); a.value(4,0x0); a.call(0x10274170); a.value(0,0x10);
                    a.store<2>(15,0x19c,0); a.jump(0x10281748);
                    primary_phase_508(a,state);
                } else {
                    a.at(0x10280ec8); a.jump(0x102816e0);
                    primary_phase_507(a,state);
                }
            }
        }
    }
}
void primary_phase_475(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280ea8); a.value(14,0x3);
    primary_phase_476(a,state);
}
void primary_phase_474(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280e84); a.sign8(0,16); a.compare(0,4);
    if (!a.equal()) {
        a.at(0x10280eac);
        primary_phase_476(a,state);
    } else {
        a.at(0x10280e90); a.offset(0,14,-0x1); a.mask(0,0,0xff,false,0); a.compare(0,1,false);
        if (a.nonpositive()) {
            a.at(0x10280ea8);
            primary_phase_475(a,state);
        } else {
            a.at(0x10280ea0); a.sign8(0,18,true);
            if (!a.equal()) {
                a.at(0x10280eac);
                primary_phase_476(a,state);
            } else {
                a.at(0x10280ea8);
                primary_phase_475(a,state);
            }
        }
    }
}
void primary_phase_473(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280e74); a.load<2>(0,15,0x19c,true); a.compare(0,59);
    if (a.equal()) {
        a.at(0x10280e80); a.value(14,0x3);
    }
    a.at(0x10280e84);
    primary_phase_474(a,state);
}
void primary_phase_472(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280e44); a.move(3,15); a.call(0x10285790); a.move(4,3); a.move(3,15); a.value(5,0x0);
    a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20);
    a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_471(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280e3c); a.value(14,0x4); a.jump(0x10280e74);
    primary_phase_473(a,state);
}
void primary_phase_470(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280df4); a.load(5,30,0x0); a.value(3,0x51ec0000); a.sign8(0,13); a.load(4,27,0x0); a.load<2>(5,5,0x0);
    a.offset(3,3,-0x7ae1); a.add(0,14,0); a.add(5,5,4); a.multiply_high(3,3,5); a.shift_right(3,3,0x4);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x32); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.at_least()) {
        a.at(0x10280e3c);
        primary_phase_471(a,state);
    } else {
        a.at(0x10280e34); a.value(14,0x2); a.jump(0x10280e74);
        primary_phase_473(a,state);
    }
}
void primary_phase_469(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280dcc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x10280de4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
    }
    a.at(0x10280df4);
    primary_phase_470(a,state);
}
void primary_phase_468(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280dc8); a.move(14,0);
    primary_phase_469(a,state);
}
void primary_phase_467(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280cb8); a.exclusive_flags(3,17,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,17,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,17,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10280d94); a.offset(14,17,-0x1); a.jump(0x10280dcc);
    }
    a.at(0x10280dcc);
    primary_phase_469(a,state);
}
void primary_phase_466(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280c98); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(17,3);
    if (a.positive()) {
        a.at(0x10280cb8);
        primary_phase_467(a,state);
    } else {
        a.at(0x10280ca8); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(17,3);
        if (a.at_least()) {
            a.at(0x10280d9c); a.load(3,30,0x0); a.compare(17,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,17); a.product(0,0,17); a.subtract(14,0,3); a.negate(0,14);
            if (a.positive()) {
                a.at(0x10280dc4); a.move(0,14);
            }
            a.at(0x10280dc8);
            primary_phase_468(a,state);
        } else {
            a.at(0x10280cb8);
            primary_phase_467(a,state);
        }
    }
}
void primary_phase_465(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280c04); a.sign8(0,16); a.compare(0,4);
    if (a.equal()) {
        a.at(0x10280e74);
        primary_phase_473(a,state);
    } else {
        a.at(0x10280c10); a.compare(22,0,false);
        if (a.equal()) {
            a.at(0x10280c20); a.move(3,15); a.call(0x10285790); a.sign8(0,3); a.compare(0,3);
            if (a.at_least()) {
                a.at(0x10280e44);
                primary_phase_472(a,state);
            } else {
                a.at(0x10280c34); a.floating_load(0,26,144); a.floating_compare(29,0);
                if (a.nonpositive()) {
                    a.at(0x10280e44);
                    primary_phase_472(a,state);
                } else {
                    a.at(0x10280c40); a.load<2>(0,15,0x19c,true); a.compare(0,2);
                    if (a.equal()) {
                        a.at(0x10280e3c);
                        primary_phase_471(a,state);
                    } else {
                        a.at(0x10280c4c); a.floating_load(0,15,152,true); a.load<1>(13,15,0x10c);
                        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.sign16(17,0,true);
                        if (!a.equal()) {
                            a.at(0x10280c70); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                            a.compare_registers(3,0,false); a.store(30,0x0,3);
                            if (a.positive()) {
                                a.at(0x10280c88); a.store(30,0x0,28); a.call(0x1077b770);
                                a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
                            }
                            a.at(0x10280c98);
                            primary_phase_466(a,state);
                        } else {
                            a.at(0x10280c68); a.value(14,0x0); a.jump(0x10280dcc);
                            primary_phase_469(a,state);
                        }
                    }
                }
            }
        } else {
            a.at(0x10280c18); a.value(14,0x1); a.jump(0x10280e74);
            primary_phase_473(a,state);
        }
    }
}
void primary_phase_464(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280bc4); a.load(5,30,0x0); a.value(3,0x88890000); a.load(4,27,0x0); a.offset(3,3,-0x7777);
    a.load<2>(5,5,0x0); a.sign8(0,13); a.add(5,5,4); a.multiply_high(3,3,5); a.add(3,3,5); a.shift_right(3,3,0x6);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x78); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.at_least()) {
        a.at(0x10280e74);
        primary_phase_473(a,state);
    } else {
        a.at(0x10280c04);
        primary_phase_465(a,state);
    }
}
void primary_phase_463(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b90); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10280e74);
        primary_phase_473(a,state);
    } else {
        a.at(0x10280b98); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.load<1>(13,15,0x10c);
        a.compare_registers(3,0,false); a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10280bb4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10280bc4);
        primary_phase_464(a,state);
    }
}
void primary_phase_462(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b84); a.value(0,0x1); a.jump(0x10280b90);
    primary_phase_463(a,state);
}
void primary_phase_461(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b5c); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10280c04);
        primary_phase_465(a,state);
    } else {
        a.at(0x10280b64); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x10280b84);
            primary_phase_462(a,state);
        } else {
            a.at(0x10280b74); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x10280b8c); a.value(0,0x0);
                primary_phase_463(a,state);
            } else {
                a.at(0x10280b84);
                primary_phase_462(a,state);
            }
        }
    }
}
void primary_phase_460(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b58); a.value(0,0x0);
    primary_phase_461(a,state);
}
void primary_phase_459(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b50); a.value(0,0x1); a.jump(0x10280b5c);
    primary_phase_461(a,state);
}
void primary_phase_458(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b34); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10280b58);
        primary_phase_460(a,state);
    } else {
        a.at(0x10280b40); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x10280b58);
            primary_phase_460(a,state);
        } else {
            a.at(0x10280b50);
            primary_phase_459(a,state);
        }
    }
}
void primary_phase_457(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b0c); a.sign8(0,14); a.compare(0,3);
    if (!a.equal()) {
        a.at(0x10280e74);
        primary_phase_473(a,state);
    } else {
        a.at(0x10280b18); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x10280b34);
            primary_phase_458(a,state);
        } else {
            a.at(0x10280b24); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x10280b50);
                primary_phase_459(a,state);
            } else {
                a.at(0x10280b34);
                primary_phase_458(a,state);
            }
        }
    }
}
void primary_phase_456(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280b08); a.value(14,0x1);
    primary_phase_457(a,state);
}
void primary_phase_455(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280ad8); a.sign8(0,14); a.compare(0,4);
    if (!a.equal()) {
        a.at(0x10280b0c);
        primary_phase_457(a,state);
    } else {
        a.at(0x10280ae4); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
        if (a.equal()) {
            a.at(0x10280b08);
            primary_phase_456(a,state);
        } else {
            a.at(0x10280af4); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
            if (!a.equal()) {
                a.at(0x10280b0c);
                primary_phase_457(a,state);
            } else {
                a.at(0x10280b08);
                primary_phase_456(a,state);
            }
        }
    }
}
void primary_phase_454(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280ad4); a.value(14,0x1);
    primary_phase_455(a,state);
}
void primary_phase_453(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280ac0); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
    if (!a.equal()) {
        a.at(0x10280ad8);
        primary_phase_455(a,state);
    } else {
        a.at(0x10280ad4);
        primary_phase_454(a,state);
    }
}
void primary_phase_452(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280a94); a.sign8(0,14); a.compare(0,3);
    if (!a.equal()) {
        a.at(0x10280ad8);
        primary_phase_455(a,state);
    } else {
        a.at(0x10280aa0); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x10280ac0);
            primary_phase_453(a,state);
        } else {
            a.at(0x10280ab0); a.floating_load(0,26,200); a.floating_compare(31,0);
            a.compare_greater_or_equal_bit();
            if (a.equal()) {
                a.at(0x10280ad4);
                primary_phase_454(a,state);
            } else {
                a.at(0x10280ac0);
                primary_phase_453(a,state);
            }
        }
    }
}
void primary_phase_451(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280a70); a.value(14,0x2); a.jump(0x10280a94);
    primary_phase_452(a,state);
}
void primary_phase_450(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280a1c); a.load(5,30,0x0); a.value(3,0x88890000); a.load(4,27,0x0); a.offset(3,3,-0x7777);
    a.load<2>(5,5,0x0); a.sign8(0,13); a.add(5,5,4); a.multiply_high(3,3,5); a.add(3,3,5); a.shift_right(3,3,0x3);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0xf); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.nonpositive()) {
        a.at(0x10280a70);
        primary_phase_451(a,state);
    } else {
        a.at(0x10280a5c); a.floating_load(0,26,632); a.floating_compare(31,0);
        if (a.at_least()) {
            a.at(0x10280a70);
            primary_phase_451(a,state);
        } else {
            a.at(0x10280a68); a.value(14,0x3); a.jump(0x10280a94);
            primary_phase_452(a,state);
        }
    }
}
void primary_phase_449(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102809d8); a.sign8(0,3,true);
    if (!a.equal()) {
        a.at(0x10280a94);
        primary_phase_452(a,state);
    } else {
        a.at(0x102809e0); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,3);
        if (!a.equal()) {
            a.at(0x10280a78); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
            if (!a.equal()) {
                a.at(0x10280a90); a.value(14,0x4);
            } else {
                a.at(0x10280a88); a.value(14,0x1); a.jump(0x10280a94);
            }
            a.at(0x10280a94);
            primary_phase_452(a,state);
        } else {
            a.at(0x102809f0); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.load<1>(13,15,0x10c);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10280a0c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x10280a1c);
            primary_phase_450(a,state);
        }
    }
}
void primary_phase_448(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102809b8); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0x0); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60);
    primary_phase_449(a,state);
}
void primary_phase_447(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280960); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0xb); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60); a.jump(0x102809d8);
    primary_phase_449(a,state);
}
void primary_phase_446(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280914); a.sign8(0,14); a.compare(0,3);
    if (!a.equal()) {
        a.at(0x10280a94);
        primary_phase_452(a,state);
    } else {
        a.at(0x10280920); a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10280984); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x102809b8);
                primary_phase_448(a,state);
            } else {
                a.at(0x10280990); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x102809b8);
                    primary_phase_448(a,state);
                } else {
                    a.at(0x102809a0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x102809b8);
                        primary_phase_448(a,state);
                    } else {
                        a.at(0x102809b0); a.value(3,0x1); a.jump(0x102809d8);
                        primary_phase_449(a,state);
                    }
                }
            }
        } else {
            a.at(0x1028092c); a.load<1>(0,15,0x106); a.compare(0,11);
            if (!a.equal()) {
                a.at(0x10280960);
                primary_phase_447(a,state);
            } else {
                a.at(0x10280938); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x10280960);
                    primary_phase_447(a,state);
                } else {
                    a.at(0x10280948); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x10280960);
                        primary_phase_447(a,state);
                    } else {
                        a.at(0x10280958); a.value(3,0x1); a.jump(0x102809d8);
                        primary_phase_449(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_445(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102808e4); a.move(3,15); a.call(0x10285790); a.move(4,3); a.move(3,15); a.value(5,0x0);
    a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20);
    a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_444(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102808dc); a.sign8(14,0); a.jump(0x10280914);
    primary_phase_446(a,state);
}
void primary_phase_443(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102808cc); a.compare(14,0); a.value(0,0x4);
    if (a.equal()) {
        a.at(0x102808d8); a.value(0,0x2);
    }
    a.at(0x102808dc);
    primary_phase_444(a,state);
}
void primary_phase_442(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102808c8); a.move(14,0);
    primary_phase_443(a,state);
}
void primary_phase_441(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102807b8); a.exclusive_flags(3,13,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,13,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,13,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10280894); a.offset(14,13,-0x1); a.jump(0x102808cc);
    }
    a.at(0x102808cc);
    primary_phase_443(a,state);
}
void primary_phase_440(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280798); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(13,3);
    if (a.positive()) {
        a.at(0x102807b8);
        primary_phase_441(a,state);
    } else {
        a.at(0x102807a8); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(13,3);
        if (a.at_least()) {
            a.at(0x1028089c); a.load(3,30,0x0); a.compare(13,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,13); a.product(0,0,13); a.subtract(14,0,3); a.negate(0,14);
            if (a.positive()) {
                a.at(0x102808c4); a.move(0,14);
            }
            a.at(0x102808c8);
            primary_phase_442(a,state);
        } else {
            a.at(0x102807b8);
            primary_phase_441(a,state);
        }
    }
}
void primary_phase_439(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280760); a.compare(13,0);
    if (!a.equal()) {
        a.at(0x10280770); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10280788); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10280798);
        primary_phase_440(a,state);
    } else {
        a.at(0x10280768); a.value(14,0x0); a.jump(0x102808cc);
        primary_phase_443(a,state);
    }
}
void primary_phase_438(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028073c); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
    if (a.equal()) {
        a.at(0x102808e4);
        primary_phase_445(a,state);
    } else {
        a.at(0x1028074c); a.load<2>(0,15,0x19c,true); a.value(13,0x2); a.compare(0,16);
        if (a.equal()) {
            a.at(0x1028075c); a.value(13,0xa);
        }
        a.at(0x10280760);
        primary_phase_439(a,state);
    }
}
void primary_phase_437(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280734); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102808e4);
        primary_phase_445(a,state);
    } else {
        a.at(0x1028073c);
        primary_phase_438(a,state);
    }
}
void primary_phase_436(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280730); a.value(0,0x0);
    primary_phase_437(a,state);
}
void primary_phase_435(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280728); a.value(0,0x1); a.jump(0x10280734);
    primary_phase_437(a,state);
}
void primary_phase_434(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028070c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10280730);
        primary_phase_436(a,state);
    } else {
        a.at(0x10280718); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x10280730);
            primary_phase_436(a,state);
        } else {
            a.at(0x10280728);
            primary_phase_435(a,state);
        }
    }
}
void primary_phase_433(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102806bc); a.load(0,1,0x68); a.compare_registers(13,0);
    if (a.at_least()) {
        a.at(0x10280914);
        primary_phase_446(a,state);
    } else {
        a.at(0x102806c8); a.sign8(0,16); a.compare(0,4);
        if (a.equal()) {
            a.at(0x10280914);
            primary_phase_446(a,state);
        } else {
            a.at(0x102806d4); a.compare(22,0,false);
            if (a.equal()) {
                a.at(0x102806e4); a.load<1>(0,1,0x100); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x1028073c);
                    primary_phase_438(a,state);
                } else {
                    a.at(0x102806f0); a.load<1>(0,15,0x27); a.compare(0,1);
                    if (!a.equal()) {
                        a.at(0x1028070c);
                        primary_phase_434(a,state);
                    } else {
                        a.at(0x102806fc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
                        if (a.less()) {
                            a.at(0x10280728);
                            primary_phase_435(a,state);
                        } else {
                            a.at(0x1028070c);
                            primary_phase_434(a,state);
                        }
                    }
                }
            } else {
                a.at(0x102806dc); a.value(14,0x1); a.jump(0x10280914);
                primary_phase_446(a,state);
            }
        }
    }
}
void primary_phase_432(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280694); a.load<2>(0,15,0x19c,true); a.compare(0,16);
    if (a.equal()) {
        a.at(0x102806bc);
        primary_phase_433(a,state);
    } else {
        a.at(0x102806a0); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (!a.equal()) {
            a.at(0x102806bc);
            primary_phase_433(a,state);
        } else {
            a.at(0x102806b4); a.value(14,0x2); a.jump(0x10280914);
            primary_phase_446(a,state);
        }
    }
}
void primary_phase_431(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028068c); a.move(13,20); a.value(14,0x2);
    primary_phase_432(a,state);
}
void primary_phase_430(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028061c); a.compare_registers(13,20);
    if (a.at_least()) {
        a.at(0x10280694);
        primary_phase_432(a,state);
    } else {
        a.at(0x10280624); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
        if (a.equal()) {
            a.at(0x10280694);
            primary_phase_432(a,state);
        } else {
            a.at(0x10280634); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
            if (a.equal()) {
                a.at(0x10280694);
                primary_phase_432(a,state);
            } else {
                a.at(0x10280648); a.load(3,15,0x1a4); a.move(4,15); a.value(5,0x1); a.call(0x101d8d70);
                a.sign8(0,3,true);
                if (!a.equal()) {
                    a.at(0x10280694);
                    primary_phase_432(a,state);
                } else {
                    a.at(0x10280660); a.load<2>(0,15,0x19c,true); a.compare(0,16);
                    if (!a.equal()) {
                        a.at(0x1028068c);
                        primary_phase_431(a,state);
                    } else {
                        a.at(0x1028066c); a.load<1>(0,15,0x27); a.load(3,15,0x1a4); a.sign8(0,0); a.add(3,3,0);
                        a.load<1>(0,3,0x77bc); a.sign8(0,0); a.compare(0,12);
                        if (a.at_least()) {
                            a.at(0x10280694);
                            primary_phase_432(a,state);
                        } else {
                            a.at(0x1028068c);
                            primary_phase_431(a,state);
                        }
                    }
                }
            }
        }
    }
}
void primary_phase_429(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280614); a.move(13,19); a.value(14,0x4);
    primary_phase_430(a,state);
}
void primary_phase_428(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102805f0); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10280614);
        primary_phase_429(a,state);
    } else {
        a.at(0x102805fc); a.load<1>(0,15,0x105); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x10280614);
            primary_phase_429(a,state);
        } else {
            a.at(0x10280608); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x1028061c);
                primary_phase_430(a,state);
            } else {
                a.at(0x10280614);
                primary_phase_429(a,state);
            }
        }
    }
}
void primary_phase_427(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280594); a.add(21,21,14); a.value(14,0x3); a.move(13,21); a.compare_registers(13,19);
    if (a.at_least()) {
        a.at(0x1028061c);
        primary_phase_430(a,state);
    } else {
        a.at(0x102805a8); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
        if (a.equal()) {
            a.at(0x1028061c);
            primary_phase_430(a,state);
        } else {
            a.at(0x102805b8); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
            if (a.equal()) {
                a.at(0x1028061c);
                primary_phase_430(a,state);
            } else {
                a.at(0x102805cc); a.load<1>(0,15,0x27); a.sign8(0,0,true);
                if (!a.equal()) {
                    a.at(0x102805f0);
                    primary_phase_428(a,state);
                } else {
                    a.at(0x102805d8); a.load<1>(0,15,0x105); a.compare(0,4);
                    if (!a.equal()) {
                        a.at(0x102805f0);
                        primary_phase_428(a,state);
                    } else {
                        a.at(0x102805e4); a.load<1>(0,15,0x106); a.compare(0,11);
                        if (a.equal()) {
                            a.at(0x1028061c);
                            primary_phase_430(a,state);
                        } else {
                            a.at(0x102805f0);
                            primary_phase_428(a,state);
                        }
                    }
                }
            }
        }
    }
}
void primary_phase_426(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280590); a.move(14,3);
    primary_phase_427(a,state);
}
void primary_phase_425(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280438); a.load(3,30,0x0); a.value(9,0x80010000); a.value(5,0x43300000); a.value(6,0x43300000);
    a.load<2>(4,3,0x0); a.value(8,0x10000); a.load(0,27,0x0); a.offset(9,9,-0x7fff); a.sign8(7,13);
    a.store(1,0xac,5); a.add(0,4,0); a.value(4,0x80010000); a.multiply_high(9,9,0); a.store(1,0xa4,6);
    a.floating_load(3,26,128); a.offset(8,8,-0x1); a.floating_load(0,26,0); a.sign8(3,13); a.add(6,9,0);
    a.floating_load(1,26,128); a.multiply(5,7,0x32); a.offset(4,4,-0x7fff); a.shift_right(6,6,0xf);
    a.subtract_from(5,5,0x3e8); a.mask(7,6,0x1,false,1); a.exclusive_flags(5,5,0x80000000); a.store(1,0xa8,5);
    a.add(6,6,7); a.product(5,6,8); a.floating_load(2,1,164); a.floating_subtract(2,2,3); a.subtract(14,5,0);
    a.floating_difference(2,2,0,true); a.exclusive_flags(0,14,0x80000000); a.store(1,0xb0,0); a.multiply(0,3,0x32);
    a.floating_load(0,1,172); a.subtract_from(0,0,0x3e8); a.floating_subtract(0,0,1); a.multiply(0,0,0xa);
    a.floating_product(0,2,0); a.multiply_high(3,4,0); a.floating_integer(0,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_store(0,1,180); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.sign8(0,13); a.offset(3,3,0x5); a.offset(4,4,0x6667);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(4,3,4); a.multiply(0,0,0x32);
    a.add(14,14,4); a.subtract_from(3,0,0x3e8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10280538); a.sign8(0,13); a.multiply(0,0,0x32); a.subtract_from(3,0,0x3e8); a.offset(14,3,-0x1);
        a.jump(0x10280594);
    }
    a.at(0x10280594);
    primary_phase_427(a,state);
}
void primary_phase_424(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280400); a.sign8(0,13); a.value(3,0x10000); a.multiply(0,0,0x32); a.offset(3,3,-0x1);
    a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10280438);
        primary_phase_425(a,state);
    } else {
        a.at(0x1028041c); a.sign8(0,13); a.value(3,0xffff0000); a.multiply(0,0,0x32); a.offset(3,3,0x1);
        a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1028054c); a.sign8(0,13); a.load(4,30,0x0); a.multiply(3,0,0x32); a.sign8(0,13);
            a.load<2>(6,4,0x0); a.load(5,27,0x0); a.subtract_from(4,3,0x3e8); a.add(5,6,5); a.divide(3,5,4);
            a.product(3,3,4); a.multiply(0,0,0x32); a.subtract(14,3,5); a.subtract_from(0,0,0x3e8); a.negate(3,14);
            a.compare(0,0);
            if (a.positive()) {
                a.at(0x1028058c); a.move(3,14);
            }
            a.at(0x10280590);
            primary_phase_426(a,state);
        } else {
            a.at(0x10280438);
            primary_phase_425(a,state);
        }
    }
}
void primary_phase_423(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102803b4); a.load<1>(13,15,0x109); a.add(20,20,14); a.sign8(0,13); a.multiply(0,0,0x32);
    a.subtract_from(0,0,0x3e8); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x102803d8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x102803f0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10280400);
        primary_phase_424(a,state);
    } else {
        a.at(0x102803d0); a.value(14,0x0); a.jump(0x10280594);
        primary_phase_427(a,state);
    }
}
void primary_phase_422(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102803b0); a.move(14,3);
    primary_phase_423(a,state);
}
void primary_phase_421(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280258); a.load(3,30,0x0); a.value(9,0x80010000); a.value(5,0x43300000); a.value(6,0x43300000);
    a.load<2>(4,3,0x0); a.value(8,0x10000); a.load(0,27,0x0); a.offset(9,9,-0x7fff); a.sign8(7,13);
    a.store(1,0xac,5); a.add(0,4,0); a.value(4,0x80010000); a.multiply_high(9,9,0); a.store(1,0xa4,6);
    a.floating_load(3,26,128); a.offset(8,8,-0x1); a.floating_load(0,26,0); a.sign8(3,13); a.add(6,9,0);
    a.floating_load(1,26,128); a.multiply(5,7,0x32); a.offset(4,4,-0x7fff); a.shift_right(6,6,0xf);
    a.subtract_from(5,5,0x3e8); a.mask(7,6,0x1,false,1); a.exclusive_flags(5,5,0x80000000); a.store(1,0xa8,5);
    a.add(6,6,7); a.product(5,6,8); a.floating_load(2,1,164); a.floating_subtract(2,2,3); a.subtract(14,5,0);
    a.floating_difference(2,2,0,true); a.exclusive_flags(0,14,0x80000000); a.store(1,0xb0,0); a.multiply(0,3,0x32);
    a.floating_load(0,1,172); a.subtract_from(0,0,0x3e8); a.floating_subtract(0,0,1); a.multiply(0,0,0xa);
    a.floating_product(0,2,0); a.multiply_high(3,4,0); a.floating_integer(0,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_store(0,1,180); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.sign8(0,13); a.offset(3,3,0x5); a.offset(4,4,0x6667);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(4,3,4); a.multiply(0,0,0x32);
    a.add(14,14,4); a.subtract_from(3,0,0x3e8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10280358); a.sign8(0,13); a.multiply(0,0,0x32); a.subtract_from(3,0,0x3e8); a.offset(14,3,-0x1);
        a.jump(0x102803b4);
    }
    a.at(0x102803b4);
    primary_phase_423(a,state);
}
void primary_phase_420(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280220); a.sign8(0,13); a.value(3,0x10000); a.multiply(0,0,0x32); a.offset(3,3,-0x1);
    a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10280258);
        primary_phase_421(a,state);
    } else {
        a.at(0x1028023c); a.sign8(0,13); a.value(3,0xffff0000); a.multiply(0,0,0x32); a.offset(3,3,0x1);
        a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1028036c); a.sign8(0,13); a.load(4,30,0x0); a.multiply(3,0,0x32); a.sign8(0,13);
            a.load<2>(6,4,0x0); a.load(5,27,0x0); a.subtract_from(4,3,0x3e8); a.add(5,6,5); a.divide(3,5,4);
            a.product(3,3,4); a.multiply(0,0,0x32); a.subtract(14,3,5); a.subtract_from(0,0,0x3e8); a.negate(3,14);
            a.compare(0,0);
            if (a.positive()) {
                a.at(0x102803ac); a.move(3,14);
            }
            a.at(0x102803b0);
            primary_phase_422(a,state);
        } else {
            a.at(0x10280258);
            primary_phase_421(a,state);
        }
    }
}
void primary_phase_419(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102801d4); a.load<1>(13,15,0x109); a.add(19,19,14); a.sign8(0,13); a.multiply(0,0,0x32);
    a.subtract_from(0,0,0x3e8); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x102801f8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10280210); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10280220);
        primary_phase_420(a,state);
    } else {
        a.at(0x102801f0); a.value(14,0x0); a.jump(0x102803b4);
        primary_phase_423(a,state);
    }
}
void primary_phase_418(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102801d0); a.move(14,3);
    primary_phase_419(a,state);
}
void primary_phase_417(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280078); a.load(3,30,0x0); a.value(9,0x80010000); a.value(5,0x43300000); a.value(6,0x43300000);
    a.load<2>(4,3,0x0); a.value(8,0x10000); a.load(0,27,0x0); a.offset(9,9,-0x7fff); a.sign8(7,13);
    a.store(1,0xac,5); a.add(0,4,0); a.value(4,0x80010000); a.multiply_high(9,9,0); a.store(1,0xa4,6);
    a.floating_load(3,26,128); a.offset(8,8,-0x1); a.floating_load(0,26,0); a.sign8(3,13); a.add(6,9,0);
    a.floating_load(1,26,128); a.multiply(5,7,0x32); a.offset(4,4,-0x7fff); a.shift_right(6,6,0xf);
    a.subtract_from(5,5,0x3e8); a.mask(7,6,0x1,false,1); a.exclusive_flags(5,5,0x80000000); a.store(1,0xa8,5);
    a.add(6,6,7); a.product(5,6,8); a.floating_load(2,1,164); a.floating_subtract(2,2,3); a.subtract(14,5,0);
    a.floating_difference(2,2,0,true); a.exclusive_flags(0,14,0x80000000); a.store(1,0xb0,0); a.multiply(0,3,0x32);
    a.floating_load(0,1,172); a.subtract_from(0,0,0x3e8); a.floating_subtract(0,0,1); a.multiply(0,0,0xa);
    a.floating_product(0,2,0); a.multiply_high(3,4,0); a.floating_integer(0,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_store(0,1,180); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.sign8(0,13); a.offset(3,3,0x5); a.offset(4,4,0x6667);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(4,3,4); a.multiply(0,0,0x32);
    a.add(14,14,4); a.subtract_from(3,0,0x3e8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10280178); a.sign8(0,13); a.multiply(0,0,0x32); a.subtract_from(3,0,0x3e8); a.offset(14,3,-0x1);
        a.jump(0x102801d4);
    }
    a.at(0x102801d4);
    primary_phase_419(a,state);
}
void primary_phase_416(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10280040); a.sign8(0,13); a.value(3,0x10000); a.multiply(0,0,0x32); a.offset(3,3,-0x1);
    a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10280078);
        primary_phase_417(a,state);
    } else {
        a.at(0x1028005c); a.sign8(0,13); a.value(3,0xffff0000); a.multiply(0,0,0x32); a.offset(3,3,0x1);
        a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1028018c); a.sign8(0,13); a.load(4,30,0x0); a.multiply(3,0,0x32); a.sign8(0,13);
            a.load<2>(6,4,0x0); a.load(5,27,0x0); a.subtract_from(4,3,0x3e8); a.add(5,6,5); a.divide(3,5,4);
            a.product(3,3,4); a.multiply(0,0,0x32); a.subtract(14,3,5); a.subtract_from(0,0,0x3e8); a.negate(3,14);
            a.compare(0,0);
            if (a.positive()) {
                a.at(0x102801cc); a.move(3,14);
            }
            a.at(0x102801d0);
            primary_phase_418(a,state);
        } else {
            a.at(0x10280078);
            primary_phase_417(a,state);
        }
    }
}
void primary_phase_415(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ffec); a.load(0,1,0x68); a.add(0,0,14); a.store(1,0x68,0); a.load<1>(13,15,0x109); a.sign8(0,13);
    a.multiply(0,0,0x32); a.subtract_from(0,0,0x3e8); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x10280018); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10280030); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10280040);
        primary_phase_416(a,state);
    } else {
        a.at(0x10280010); a.value(14,0x0); a.jump(0x102801d4);
        primary_phase_419(a,state);
    }
}
void primary_phase_414(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ffe8); a.move(14,3);
    primary_phase_415(a,state);
}
void primary_phase_413(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fe90); a.load(3,30,0x0); a.value(9,0x80010000); a.value(5,0x43300000); a.value(6,0x43300000);
    a.load<2>(4,3,0x0); a.value(8,0x10000); a.load(0,27,0x0); a.offset(9,9,-0x7fff); a.sign8(7,13);
    a.store(1,0xac,5); a.add(0,4,0); a.value(4,0x80010000); a.multiply_high(9,9,0); a.store(1,0xa4,6);
    a.floating_load(3,26,128); a.offset(8,8,-0x1); a.floating_load(0,26,0); a.sign8(3,13); a.add(6,9,0);
    a.floating_load(1,26,128); a.multiply(5,7,0x32); a.offset(4,4,-0x7fff); a.shift_right(6,6,0xf);
    a.subtract_from(5,5,0x3e8); a.mask(7,6,0x1,false,1); a.exclusive_flags(5,5,0x80000000); a.store(1,0xa8,5);
    a.add(6,6,7); a.product(5,6,8); a.floating_load(2,1,164); a.floating_subtract(2,2,3); a.subtract(14,5,0);
    a.floating_difference(2,2,0,true); a.exclusive_flags(0,14,0x80000000); a.store(1,0xb0,0); a.multiply(0,3,0x32);
    a.floating_load(0,1,172); a.subtract_from(0,0,0x3e8); a.floating_subtract(0,0,1); a.multiply(0,0,0xa);
    a.floating_product(0,2,0); a.multiply_high(3,4,0); a.floating_integer(0,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_store(0,1,180); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.sign8(0,13); a.offset(3,3,0x5); a.offset(4,4,0x6667);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(4,3,4); a.multiply(0,0,0x32);
    a.add(14,14,4); a.subtract_from(3,0,0x3e8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027ff90); a.sign8(0,13); a.multiply(0,0,0x32); a.subtract_from(3,0,0x3e8); a.offset(14,3,-0x1);
        a.jump(0x1027ffec);
    }
    a.at(0x1027ffec);
    primary_phase_415(a,state);
}
void primary_phase_412(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fe58); a.sign8(0,13); a.value(3,0x10000); a.multiply(0,0,0x32); a.offset(3,3,-0x1);
    a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027fe90);
        primary_phase_413(a,state);
    } else {
        a.at(0x1027fe74); a.sign8(0,13); a.value(3,0xffff0000); a.multiply(0,0,0x32); a.offset(3,3,0x1);
        a.subtract_from(0,0,0x3e8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027ffa4); a.sign8(0,13); a.load(4,30,0x0); a.multiply(3,0,0x32); a.sign8(0,13);
            a.load<2>(6,4,0x0); a.load(5,27,0x0); a.subtract_from(4,3,0x3e8); a.add(5,6,5); a.divide(3,5,4);
            a.product(3,3,4); a.multiply(0,0,0x32); a.subtract(14,3,5); a.subtract_from(0,0,0x3e8); a.negate(3,14);
            a.compare(0,0);
            if (a.positive()) {
                a.at(0x1027ffe4); a.move(3,14);
            }
            a.at(0x1027ffe8);
            primary_phase_414(a,state);
        } else {
            a.at(0x1027fe90);
            primary_phase_413(a,state);
        }
    }
}
void primary_phase_411(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fe10); a.load<1>(13,15,0x109); a.sign8(0,13); a.multiply(0,0,0x32); a.subtract_from(0,0,0x3e8);
    a.compare(0,0);
    if (!a.equal()) {
        a.at(0x1027fe30); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027fe48); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027fe58);
        primary_phase_412(a,state);
    } else {
        a.at(0x1027fe28); a.value(14,0x0); a.jump(0x1027ffec);
        primary_phase_415(a,state);
    }
}
void primary_phase_410(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fdac); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,27,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0xa); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x1027fe10);
        primary_phase_411(a,state);
    } else {
        a.at(0x1027fde0); a.move(3,15); a.call(0x10285790); a.move(4,3); a.move(3,15); a.value(5,0x0);
        a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20);
        a.move(3,13); a.jump(0x1028175c);
    }
}
void primary_phase_409(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fd84); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x1027fd9c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
    }
    a.at(0x1027fdac);
    primary_phase_410(a,state);
}
void primary_phase_408(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fd38); a.load(4,1,0x68); a.move(3,15); a.offset(0,4,0x5dc); a.store(1,0x68,0); a.call(0x102864b0);
    a.value(5,0x0); a.value(0,0x4); a.intersect(6,4,5); a.value(4,0x0); a.intersect(5,3,0); a.value(0,0x0);
    a.combine(3,6,4,false,true); a.combine(0,5,0,false,true); a.combine(0,3,0,true);
    if (!a.equal()) {
        a.at(0x1027fd74); a.load(3,1,0x68); a.offset(0,3,0x5dc); a.store(1,0x68,0); a.jump(0x1027fe10);
    }
    a.at(0x1027fe10);
    primary_phase_411(a,state);
}
void primary_phase_407(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fd10); a.compare(22,0,false);
    if (a.equal()) {
        a.at(0x1027fd84);
        primary_phase_409(a,state);
    } else {
        a.at(0x1027fd18); a.load<1>(3,1,0x50); a.offset(0,3,-0x7); a.mask(0,0,0xff,false,0); a.compare(0,1,false);
        if (a.nonpositive()) {
            a.at(0x1027fd38);
            primary_phase_408(a,state);
        } else {
            a.at(0x1027fd2c); a.load<1>(0,1,0x50); a.compare(0,6);
            if (!a.equal()) {
                a.at(0x1027fd84);
                primary_phase_409(a,state);
            } else {
                a.at(0x1027fd38);
                primary_phase_408(a,state);
            }
        }
    }
}
void primary_phase_406(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fcf8); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027fe10);
        primary_phase_411(a,state);
    } else {
        a.at(0x1027fd04); a.load<1>(0,15,0x106); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027fe10);
            primary_phase_411(a,state);
        } else {
            a.at(0x1027fd10);
            primary_phase_407(a,state);
        }
    }
}
void primary_phase_405(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fcb4); a.load(3,15,0x1a4); a.subtract(19,14,19); a.load<1>(4,15,0x105); a.call(0x101ee9c0);
    a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027fe10);
        primary_phase_411(a,state);
    } else {
        a.at(0x1027fccc); a.move(3,15); a.call(0x10285790); a.sign8(0,3); a.compare(0,3);
        if (a.at_least()) {
            a.at(0x1027fe10);
            primary_phase_411(a,state);
        } else {
            a.at(0x1027fce0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027fcf8);
                primary_phase_406(a,state);
            } else {
                a.at(0x1027fcec); a.load<1>(0,15,0x106); a.compare(0,11);
                if (a.equal()) {
                    a.at(0x1027fd10);
                    primary_phase_407(a,state);
                } else {
                    a.at(0x1027fcf8);
                    primary_phase_406(a,state);
                }
            }
        }
    }
}
void primary_phase_404(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc94); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027fc9c); a.value(4,0x0); a.mask(0,14,0x1,false,1); a.subtract_register_carry(3,4,14);
        a.mask(3,4,0x1,false,1); a.subtract_extended(0,0,3); a.intersect(14,14,0,true);
    }
    a.at(0x1027fcb4);
    primary_phase_405(a,state);
}
void primary_phase_403(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc90); a.value(0,0x0);
    primary_phase_404(a,state);
}
void primary_phase_402(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc88); a.value(0,0x1); a.jump(0x1027fc94);
    primary_phase_404(a,state);
}
void primary_phase_401(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc6c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027fc90);
        primary_phase_403(a,state);
    } else {
        a.at(0x1027fc78); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027fc90);
            primary_phase_403(a,state);
        } else {
            a.at(0x1027fc88);
            primary_phase_402(a,state);
        }
    }
}
void primary_phase_400(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc50); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027fc6c);
        primary_phase_401(a,state);
    } else {
        a.at(0x1027fc5c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027fc88);
            primary_phase_402(a,state);
        } else {
            a.at(0x1027fc6c);
            primary_phase_401(a,state);
        }
    }
}
void primary_phase_399(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc4c); a.move(14,13);
    primary_phase_400(a,state);
}
void primary_phase_398(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fc48); a.move(13,0);
    primary_phase_399(a,state);
}
void primary_phase_397(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fb38); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027fc14); a.offset(13,14,-0x1); a.jump(0x1027fc4c);
    }
    a.at(0x1027fc4c);
    primary_phase_399(a,state);
}
void primary_phase_396(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fb18); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027fb38);
        primary_phase_397(a,state);
    } else {
        a.at(0x1027fb28); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027fc1c); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027fc44); a.move(0,13);
            }
            a.at(0x1027fc48);
            primary_phase_398(a,state);
        } else {
            a.at(0x1027fb38);
            primary_phase_397(a,state);
        }
    }
}
void primary_phase_395(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fad4); a.load<1>(0,15,0x109); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027fc50);
        primary_phase_400(a,state);
    } else {
        a.at(0x1027fae0); a.compare(14,0);
        if (!a.equal()) {
            a.at(0x1027faf0); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027fb08); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027fb18);
            primary_phase_396(a,state);
        } else {
            a.at(0x1027fae8); a.value(13,0x0); a.jump(0x1027fc4c);
            primary_phase_399(a,state);
        }
    }
}
void primary_phase_394(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027fad0); a.add(14,14,13);
    primary_phase_395(a,state);
}
void primary_phase_393(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027facc); a.move(13,0);
    primary_phase_394(a,state);
}
void primary_phase_392(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f9bc); a.exclusive_flags(3,21,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,21,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,21,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027fa98); a.offset(13,21,-0x1); a.jump(0x1027fad0);
    }
    a.at(0x1027fad0);
    primary_phase_394(a,state);
}
void primary_phase_391(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f99c); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(21,3);
    if (a.positive()) {
        a.at(0x1027f9bc);
        primary_phase_392(a,state);
    } else {
        a.at(0x1027f9ac); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(21,3);
        if (a.at_least()) {
            a.at(0x1027faa0); a.load(3,30,0x0); a.compare(21,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,21); a.product(0,0,21); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027fac8); a.move(0,13);
            }
            a.at(0x1027facc);
            primary_phase_393(a,state);
        } else {
            a.at(0x1027f9bc);
            primary_phase_392(a,state);
        }
    }
}
void primary_phase_390(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f944); a.load<1>(0,1,0xfc); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027fad4);
        primary_phase_395(a,state);
    } else {
        a.at(0x1027f950); a.compare(21,2000);
        if (a.nonpositive()) {
            a.at(0x1027fad4);
            primary_phase_395(a,state);
        } else {
            a.at(0x1027f958); a.load<2>(0,15,0x2a,true); a.compare(0,-50);
            if (a.nonpositive()) {
                a.at(0x1027fad4);
                primary_phase_395(a,state);
            } else {
                a.at(0x1027f964); a.compare(21,0);
                if (!a.equal()) {
                    a.at(0x1027f974); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027f98c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027f99c);
                    primary_phase_391(a,state);
                } else {
                    a.at(0x1027f96c); a.value(13,0x0); a.jump(0x1027fad0);
                    primary_phase_394(a,state);
                }
            }
        }
    }
}
void primary_phase_389(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f924); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027f934); a.floating_load(0,26,24); a.floating_compare(31,0);
        if (a.at_least()) {
        } else {
            a.at(0x1027f940); a.offset(19,19,0x1770);
        }
    } else {
        a.at(0x1027f92c); a.offset(19,19,0x7d0); a.jump(0x1027f944);
    }
    a.at(0x1027f944);
    primary_phase_390(a,state);
}
void primary_phase_388(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f920); a.value(0,0x0);
    primary_phase_389(a,state);
}
void primary_phase_387(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f918); a.value(0,0x1); a.jump(0x1027f924);
    primary_phase_389(a,state);
}
void primary_phase_386(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f8fc); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027f920);
        primary_phase_388(a,state);
    } else {
        a.at(0x1027f908); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027f920);
            primary_phase_388(a,state);
        } else {
            a.at(0x1027f918);
            primary_phase_387(a,state);
        }
    }
}
void primary_phase_385(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f8d8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027f944);
        primary_phase_390(a,state);
    } else {
        a.at(0x1027f8e0); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027f8fc);
            primary_phase_386(a,state);
        } else {
            a.at(0x1027f8ec); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027f918);
                primary_phase_387(a,state);
            } else {
                a.at(0x1027f8fc);
                primary_phase_386(a,state);
            }
        }
    }
}
void primary_phase_384(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f8cc); a.value(0,0x1); a.jump(0x1027f8d8);
    primary_phase_385(a,state);
}
void primary_phase_383(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f89c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027f8ac); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x1027f8cc);
            primary_phase_384(a,state);
        } else {
            a.at(0x1027f8bc); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x1027f8d4); a.value(0,0x0);
                primary_phase_385(a,state);
            } else {
                a.at(0x1027f8cc);
                primary_phase_384(a,state);
            }
        }
    } else {
        a.at(0x1027f8a4); a.offset(14,14,0x1f4); a.jump(0x1027f944);
        primary_phase_390(a,state);
    }
}
void primary_phase_382(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f890); a.value(0,0x1); a.jump(0x1027f89c);
    primary_phase_383(a,state);
}
void primary_phase_381(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f838); a.move(3,15); a.call(0x102864b0); a.value(5,0x0); a.value(0,0x4); a.intersect(6,4,5);
    a.value(4,0x0); a.intersect(5,3,0); a.value(0,0x0); a.combine(3,6,4,false,true); a.combine(0,5,0,false,true);
    a.combine(0,3,0,true);
    if (a.equal()) {
        a.at(0x1027f944);
        primary_phase_390(a,state);
    } else {
        a.at(0x1027f868); a.move(3,15); a.call(0x1028abc0); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x1027f944);
            primary_phase_390(a,state);
        } else {
            a.at(0x1027f878); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x1027f890);
                primary_phase_382(a,state);
            } else {
                a.at(0x1027f884); a.load<1>(0,15,0x106); a.compare(0,11);
                if (!a.equal()) {
                    a.at(0x1027f898); a.value(0,0x0);
                    primary_phase_383(a,state);
                } else {
                    a.at(0x1027f890);
                    primary_phase_382(a,state);
                }
            }
        }
    }
}
void primary_phase_380(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f82c); a.load(0,1,0x68); a.add(14,14,0); a.jump(0x1027f944);
    primary_phase_390(a,state);
}
void primary_phase_379(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f760); a.load(3,15,0x1a4); a.value(0,0x43300000); a.store(1,0xa4,0); a.value(14,0x0);
    a.load<1>(0,3,0x791f); a.floating_load(1,26,128); a.sign8(0,0); a.floating_load(2,26,8);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0xa8,0); a.floating_load(0,1,164); a.floating_subtract(1,0,1);
    a.call(0x10780fc0); a.load(2,1,0x14); a.floating_load(2,31,208,true); a.compare(22,0,false);
    a.floating_load(0,15,80,true); a.floating_load(3,26,56); a.floating_subtract(2,2,0,true);
    a.floating_load(0,31,60,true); a.load<2>(3,15,0x2a,true); a.floating_product(1,3,1);
    a.load<2>(0,15,0x1a0,true); a.floating_difference(0,2,0,true,true); a.multiply(4,3,0xa); a.multiply(0,0,0x64);
    a.floating_integer(0,0); a.floating_store(0,1,180); a.floating_integer(0,1); a.load(3,1,0xb8);
    a.floating_store(0,1,172); a.load(5,1,0xb0); a.add(14,14,5); a.subtract(14,4,14); a.add(14,14,3);
    a.subtract(14,0,14);
    if (a.equal()) {
        a.at(0x1027f838);
        primary_phase_381(a,state);
    } else {
        a.at(0x1027f7f4); a.load(0,1,0x68); a.compare(0,0);
        if (a.nonpositive()) {
            a.at(0x1027f838);
            primary_phase_381(a,state);
        } else {
            a.at(0x1027f800); a.load<1>(0,1,0x50); a.compare(0,94);
            if (a.equal()) {
                a.at(0x1027f82c);
                primary_phase_380(a,state);
            } else {
                a.at(0x1027f80c); a.load<1>(3,1,0x50); a.offset(0,3,-0x7); a.mask(0,0,0xff,false,0);
                a.compare(0,1,false);
                if (a.nonpositive()) {
                    a.at(0x1027f82c);
                    primary_phase_380(a,state);
                } else {
                    a.at(0x1027f820); a.load<1>(0,1,0x50); a.compare(0,6);
                    if (!a.equal()) {
                        a.at(0x1027f838);
                        primary_phase_381(a,state);
                    } else {
                        a.at(0x1027f82c);
                        primary_phase_380(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_378(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f4e0); a.load(4,30,0x0); a.value(3,0x51ec0000); a.load(0,27,0x0); a.offset(3,3,-0x7ae1);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x6); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0xc8); a.subtract(0,0,4); a.compare_registers(14,0);
    if (a.nonpositive()) {
        a.at(0x1027f760);
        primary_phase_379(a,state);
    } else {
        a.at(0x1027f518); a.load<1>(3,15,0x105); a.value(23,0x0); a.load<1>(0,15,0x106); a.sign8(3,3);
        a.load(4,15,0x1a4); a.sign8(0,0); a.multiply(3,3,0x450); a.multiply(0,0,0x5c); a.add(3,3,0);
        a.offset(17,3,0xa08); a.add(17,4,17); a.jump(0x1027f730);
        if (score_cell_pressure(a,state)) {
            a.at(0x1027f760);
            primary_phase_379(a,state);
        }
    }
}
void primary_phase_377(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f4b8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x1027f4d0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
    }
    a.at(0x1027f4e0);
    primary_phase_378(a,state);
}
void primary_phase_376(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f4b4); a.move(14,0);
    primary_phase_377(a,state);
}
void primary_phase_375(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f26c); a.sign8(0,13); a.value(3,0x43300000); a.exclusive_flags(0,0,0x80000000);
    a.value(6,0x43300000); a.store(1,0xa8,0); a.value(8,0x80010000); a.load(9,30,0x0); a.value(7,0x10000);
    a.store(1,0xa4,3); a.sign8(0,13); a.floating_load(1,26,128); a.exclusive_flags(3,0,0x80000000);
    a.floating_load(0,1,164); a.value(0,0x43300000); a.floating_load(2,26,288); a.value(5,0x43300000);
    a.floating_subtract(0,0,1); a.store(1,0xb4,6); a.value(4,0x80010000); a.floating_load(4,26,128);
    a.store(1,0xcc,0); a.offset(8,8,-0x7fff); a.floating_product(0,2,0); a.floating_load(3,26,0);
    a.load<2>(9,9,0x0); a.offset(7,7,-0x1); a.load(6,27,0x0); a.offset(4,4,-0x7fff); a.floating_integer(0,0);
    a.add(6,9,6); a.store(1,0xd0,3); a.multiply_high(3,8,6); a.floating_load(1,26,128); a.floating_store(0,1,172);
    a.floating_load(0,1,204); a.load(0,1,0xb0); a.floating_subtract(0,0,1); a.exclusive_flags(0,0,0x80000000);
    a.floating_load(1,26,288); a.store(1,0xb8,0); a.add(3,3,6); a.shift_right(0,3,0xf); a.floating_load(2,1,180);
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.floating_product(0,1,0); a.product(0,0,7); a.store(1,0xbc,5);
    a.floating_subtract(4,2,4); a.floating_load(2,26,128); a.floating_integer(0,0);
    a.floating_difference(3,4,3,true); a.subtract(14,0,6); a.floating_store(0,1,212);
    a.exclusive_flags(0,14,0x80000000); a.store(1,0xc0,0); a.load(0,1,0xd8); a.floating_load(1,1,188);
    a.multiply(0,0,0xa); a.floating_subtract(1,1,2); a.multiply_high(3,4,0); a.floating_product(0,3,1);
    a.add(0,3,0); a.floating_integer(0,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.floating_store(0,1,196); a.load(14,1,0xc8); a.call(0x10226520); a.sign8(4,13); a.value(0,0x43300000);
    a.exclusive_flags(4,4,0x80000000); a.store(1,0xdc,0); a.floating_load(1,26,128); a.offset(3,3,0x5);
    a.store(1,0xe0,4); a.value(4,0x66660000); a.offset(4,4,0x6667); a.floating_load(2,26,288);
    a.floating_load(0,1,220); a.multiply_high(0,4,3); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
    a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(14,14,0); a.floating_integer(0,0);
    a.floating_store(0,1,228); a.load(3,1,0xe8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027f3e4); a.sign8(3,13); a.value(0,0x43300000); a.exclusive_flags(3,3,0x80000000);
        a.store(1,0xec,0); a.floating_load(1,26,128); a.store(1,0xf0,3); a.floating_load(2,26,288);
        a.floating_load(0,1,236); a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.floating_integer(0,0);
        a.floating_store(0,1,244); a.load(3,1,0xf8); a.offset(14,3,-0x1); a.jump(0x1027f4b8);
    }
    a.at(0x1027f4b8);
    primary_phase_377(a,state);
}
void primary_phase_374(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f1e4); a.sign8(3,13); a.value(0,0x43300000); a.exclusive_flags(4,3,0x80000000); a.store(1,0xa4,0);
    a.value(3,0x10000); a.floating_load(1,26,128); a.store(1,0xa8,4); a.offset(3,3,-0x1);
    a.floating_load(2,26,288); a.floating_load(0,1,164); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
    a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027f26c);
        primary_phase_375(a,state);
    } else {
        a.at(0x1027f228); a.sign8(3,13); a.value(0,0x43300000); a.exclusive_flags(4,3,0x80000000);
        a.store(1,0xa4,0); a.value(3,0xffff0000); a.floating_load(1,26,128); a.store(1,0xa8,4); a.offset(3,3,0x1);
        a.floating_load(2,26,288); a.floating_load(0,1,164); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027f420); a.sign8(0,13); a.value(4,0x43300000); a.exclusive_flags(0,0,0x80000000);
            a.load(5,30,0x0); a.store(1,0xa8,0); a.sign8(0,13); a.exclusive_flags(3,0,0x80000000);
            a.value(0,0x43300000); a.store(1,0xa4,4); a.floating_load(1,26,128); a.floating_load(0,1,164);
            a.floating_load(2,26,288); a.floating_subtract(0,0,1); a.load<2>(5,5,0x0); a.load(4,27,0x0);
            a.store(1,0xb8,3); a.floating_product(2,2,0); a.add(5,5,4); a.store(1,0xb4,0);
            a.floating_load(1,26,128); a.floating_load(0,1,180); a.floating_integer(2,2);
            a.floating_subtract(0,0,1); a.floating_load(1,26,288); a.floating_store(2,1,172);
            a.floating_product(0,1,0); a.load(4,1,0xb0); a.divide(3,5,4); a.floating_integer(0,0);
            a.floating_store(0,1,188); a.load(0,1,0xc0); a.compare(0,0); a.product(0,3,4); a.subtract(14,0,5);
            a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027f4b0); a.move(0,14);
            }
            a.at(0x1027f4b4);
            primary_phase_376(a,state);
        } else {
            a.at(0x1027f26c);
            primary_phase_375(a,state);
        }
    }
}
void primary_phase_373(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f174); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027f760);
        primary_phase_379(a,state);
    } else {
        a.at(0x1027f17c); a.load<1>(13,15,0x109); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,13);
        a.floating_load(2,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(3,26,288); a.store(1,0xa8,0);
        a.floating_load(0,26,136); a.floating_load(1,1,164); a.floating_subtract(1,1,2); a.floating_product(1,3,1);
        a.floating_compare(1,0);
        if (!a.equal()) {
            a.at(0x1027f1bc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027f1d4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027f1e4);
            primary_phase_374(a,state);
        } else {
            a.at(0x1027f1b4); a.value(14,0x0); a.jump(0x1027f4b8);
            primary_phase_377(a,state);
        }
    }
}
void primary_phase_372(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f170); a.sign8(0,3);
    primary_phase_373(a,state);
}
void primary_phase_371(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f124); a.sign8(0,3); a.jump(0x1027f174);
    primary_phase_373(a,state);
}
void primary_phase_370(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f0d0); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027f12c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027f140); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027f150); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027f15c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027f16c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027f170);
        primary_phase_372(a,state);
    } else {
        a.at(0x1027f0dc); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027f0f0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027f100); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027f110); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027f120); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027f124);
        primary_phase_371(a,state);
    }
}
void primary_phase_369(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f0c8); a.offset(20,20,-0x2710); a.jump(0x1027f760);
    primary_phase_379(a,state);
}
void primary_phase_368(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f0c4); a.add(21,21,13);
    primary_phase_369(a,state);
}
void primary_phase_367(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027f0c0); a.move(13,0);
    primary_phase_368(a,state);
}
void primary_phase_366(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ef54); a.value(4,0x60000); a.value(0,0x43300000); a.sign8(3,14); a.load(8,30,0x0);
    a.offset(4,4,0x1a80); a.value(7,0x80010000); a.divide(4,4,3); a.store(1,0xa4,0); a.load<2>(9,8,0x0);
    a.value(5,0x43300000); a.load(8,27,0x0); a.value(3,0x60000); a.exclusive_flags(0,4,0x80000000);
    a.value(6,0x10000); a.store(1,0xa8,0); a.value(4,0x80010000); a.floating_load(2,26,128); a.add(8,9,8);
    a.floating_load(1,1,164); a.offset(7,7,-0x7fff); a.floating_load(0,26,0); a.sign8(0,14);
    a.floating_subtract(1,1,2); a.store(1,0xac,5); a.offset(3,3,0x1a80); a.offset(6,6,-0x1); a.divide(0,3,0);
    a.offset(4,4,-0x7fff); a.floating_difference(2,1,0,true); a.floating_load(1,26,128); a.multiply_high(5,7,8);
    a.add(3,5,8); a.shift_right(3,3,0xf); a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,6);
    a.subtract(13,3,8); a.exclusive_flags(3,13,0x80000000); a.store(1,0xb0,3); a.multiply(0,0,0xa);
    a.floating_load(0,1,172); a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
    a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
    a.floating_store(0,1,180); a.load(13,1,0xb8); a.call(0x10226520); a.value(4,0x60000); a.value(5,0x66660000);
    a.sign8(0,14); a.offset(4,4,0x1a80); a.offset(5,5,0x6667); a.divide(4,4,0); a.offset(0,3,0x5);
    a.multiply_high(3,5,0); a.offset(0,4,-0x1); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4);
    a.add(13,13,3); a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027f060); a.value(3,0x60000); a.sign8(0,14); a.offset(3,3,0x1a80); a.divide(3,3,0);
        a.offset(13,3,-0x1); a.jump(0x1027f0c4);
    }
    a.at(0x1027f0c4);
    primary_phase_368(a,state);
}
void primary_phase_365(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ef14); a.value(4,0x60000); a.value(3,0x10000); a.sign8(0,14); a.offset(4,4,0x1a80);
    a.offset(3,3,-0x1); a.divide(0,4,0); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027ef54);
        primary_phase_366(a,state);
    } else {
        a.at(0x1027ef34); a.value(4,0x60000); a.value(3,0xffff0000); a.sign8(0,14); a.offset(4,4,0x1a80);
        a.offset(3,3,0x1); a.divide(0,4,0); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027f078); a.value(3,0x60000); a.load(4,30,0x0); a.sign8(0,14); a.offset(3,3,0x1a80);
            a.load<2>(6,4,0x0); a.divide(5,3,0); a.load(4,27,0x0); a.value(3,0x60000); a.add(6,6,4); a.sign8(0,14);
            a.offset(3,3,0x1a80); a.divide(4,6,5); a.product(4,4,5);
            a.divide_record(0,3,0);
            a.subtract(13,4,6); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027f0bc); a.move(0,13);
            }
            a.at(0x1027f0c0);
            primary_phase_367(a,state);
        } else {
            a.at(0x1027ef54);
            primary_phase_366(a,state);
        }
    }
}
void primary_phase_364(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027eecc); a.sign8(14,0); a.value(3,0x60000); a.move(0,14); a.offset(3,3,0x1a80);
    a.divide_record(0,3,0);
    if (!a.equal()) {
        a.at(0x1027eeec); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027ef04); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027ef14);
        primary_phase_365(a,state);
    } else {
        a.at(0x1027eee4); a.value(13,0x0); a.jump(0x1027f0c4);
        primary_phase_368(a,state);
    }
}
void primary_phase_363(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027eeb8); a.sign8(3,13); a.value(0,0xa); a.compare(3,10);
    if (a.at_least()) {
        a.at(0x1027eec8); a.move(0,3);
    }
    a.at(0x1027eecc);
    primary_phase_364(a,state);
}
void primary_phase_362(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027eeb4); a.move(13,0);
    primary_phase_363(a,state);
}
void primary_phase_361(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ed2c); a.floating_load(0,31,92,true); a.value(0,0x43300000); a.load(7,30,0x0);
    a.value(6,0x80010000); a.floating_product_single(1,0,31); a.floating_load(0,31,92,true); a.load<2>(8,7,0x0);
    a.value(4,0x43300000); a.floating_product_single(0,0,31); a.load(7,27,0x0); a.floating_integer(1,1);
    a.value(5,0x10000); a.floating_integer(0,0); a.value(3,0x80010000); a.store(1,0xac,0); a.add(7,8,7);
    a.floating_store(1,1,164); a.offset(6,6,-0x7fff); a.floating_load(3,26,128); a.multiply_high(6,6,7);
    a.load(0,1,0xa8); a.offset(5,5,-0x1); a.store(1,0xb4,4); a.offset(3,3,-0x7fff);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0xb0,0); a.add(4,6,7); a.floating_load(2,26,0);
    a.shift_right(4,4,0xf); a.floating_store(0,1,196); a.mask(6,4,0x1,false,1); a.floating_load(0,1,172);
    a.add(4,4,6); a.product(4,4,5); a.load(0,1,0xc8); a.floating_subtract(0,0,3); a.floating_load(1,26,128);
    a.floating_difference(2,0,2,true); a.subtract(13,4,7); a.exclusive_flags(4,13,0x80000000); a.store(1,0xb8,4);
    a.multiply(0,0,0xa); a.floating_load(0,1,180); a.floating_subtract(0,0,1); a.multiply_high(3,3,0);
    a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_integer(0,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_store(0,1,188); a.load(13,1,0xc0); a.call(0x10226520);
    a.floating_load(0,31,92,true); a.value(4,0x66660000); a.offset(0,3,0x5); a.floating_product_single(0,0,31);
    a.offset(4,4,0x6667); a.multiply_high(0,4,0); a.floating_integer(0,0); a.floating_store(0,1,204);
    a.shift_right(0,0,0x2); a.mask(4,0,0x1,false,1); a.load(3,1,0xd0); a.add(0,0,4); a.add(13,13,0);
    a.offset(0,3,-0x1); a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027ee44); a.floating_load(0,31,92,true); a.floating_product_single(0,0,31);
        a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8); a.offset(13,3,-0x1);
        a.jump(0x1027eeb8);
    }
    a.at(0x1027eeb8);
    primary_phase_363(a,state);
}
void primary_phase_360(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ece4); a.floating_load(0,31,92,true); a.value(3,0x10000); a.offset(3,3,-0x1);
    a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027ed2c);
        primary_phase_361(a,state);
    } else {
        a.at(0x1027ed08); a.floating_load(0,31,92,true); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027ee60); a.floating_load(0,31,92,true); a.load(3,30,0x0); a.floating_product_single(1,0,31);
            a.floating_load(0,31,92,true); a.load<2>(3,3,0x0); a.load(0,27,0x0); a.floating_product_single(0,0,31);
            a.floating_integer(1,1); a.add(5,3,0); a.floating_integer(0,0); a.floating_store(1,1,164);
            a.load(4,1,0xa8); a.floating_store(0,1,172); a.divide(3,5,4); a.load(0,1,0xb0); a.compare(0,0);
            a.product(0,3,4); a.subtract(13,0,5); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027eeb0); a.move(0,13);
            }
            a.at(0x1027eeb4);
            primary_phase_362(a,state);
        } else {
            a.at(0x1027ed2c);
            primary_phase_361(a,state);
        }
    }
}
void primary_phase_359(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027eca0); a.floating_load(1,31,92,true); a.floating_load(0,31,0,true);
    a.floating_product_single(1,1,31); a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x1027ecbc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027ecd4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027ece4);
        primary_phase_360(a,state);
    } else {
        a.at(0x1027ecb4); a.value(13,0x0); a.jump(0x1027eeb8);
        primary_phase_363(a,state);
    }
}
void primary_phase_358(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ec6c); a.load(4,30,0x0); a.value(3,0x10620000); a.load(0,27,0x0); a.offset(3,3,0x4dd3);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x1f4); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x1027f0c8);
        primary_phase_369(a,state);
    } else {
        a.at(0x1027eca0);
        primary_phase_359(a,state);
    }
}
void primary_phase_357(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ec0c); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,4);
    if (a.at_least()) {
        a.at(0x1027f0c8);
        primary_phase_369(a,state);
    } else {
        a.at(0x1027ec20); a.sign8(0,17); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027f0c8);
            primary_phase_369(a,state);
        } else {
            a.at(0x1027ec2c); a.floating_load(0,26,24); a.floating_compare(31,0);
            if (a.less()) {
                a.at(0x1027eca0);
                primary_phase_359(a,state);
            } else {
                a.at(0x1027ec38); a.floating_load(0,26,112); a.floating_compare(31,0);
                if (a.at_least()) {
                    a.at(0x1027f0c8);
                    primary_phase_369(a,state);
                } else {
                    a.at(0x1027ec44); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027ec5c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027ec6c);
                    primary_phase_358(a,state);
                }
            }
        }
    }
}
void primary_phase_356(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ec08); a.add(19,19,13);
    primary_phase_357(a,state);
}
void primary_phase_355(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ec04); a.move(13,0);
    primary_phase_356(a,state);
}
void primary_phase_354(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027eaf4); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027ebd0); a.offset(13,14,-0x1); a.jump(0x1027ec08);
    }
    a.at(0x1027ec08);
    primary_phase_356(a,state);
}
void primary_phase_353(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ead4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027eaf4);
        primary_phase_354(a,state);
    } else {
        a.at(0x1027eae4); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027ebd8); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027ec00); a.move(0,13);
            }
            a.at(0x1027ec04);
            primary_phase_355(a,state);
        } else {
            a.at(0x1027eaf4);
            primary_phase_354(a,state);
        }
    }
}
void primary_phase_352(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ea30); a.floating_load(1,15,152,true); a.floating_load(2,26,8); a.call(0x10780fc0);
    a.load(2,1,0x14); a.load(5,1,0x68); a.value(4,0x43300000); a.load<2>(3,15,0x2a,true); a.value(0,0x43300000);
    a.subtract_from(5,5,0x2710); a.store(1,0xa4,4); a.exclusive_flags(4,5,0x80000000); a.floating_load(3,26,128);
    a.store(1,0xa8,4); a.multiply(3,3,0x3); a.floating_load(2,26,128); a.floating_load(0,1,164);
    a.exclusive_flags(3,3,0x80000000); a.store(1,0xac,0); a.floating_subtract(0,0,3); a.store(1,0xb0,3);
    a.floating_sum(1,0,1); a.floating_load(0,1,172); a.floating_subtract(0,0,2); a.floating_sum(0,0,1);
    a.floating_integer(0,0); a.floating_store(0,1,180); a.load(14,1,0xb8); a.compare(14,0);
    if (!a.equal()) {
        a.at(0x1027eaac); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027eac4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027ead4);
        primary_phase_353(a,state);
    } else {
        a.at(0x1027eaa4); a.value(13,0x0); a.jump(0x1027ec08);
        primary_phase_356(a,state);
    }
}
void primary_phase_351(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ea10); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ea30);
        primary_phase_352(a,state);
    } else {
        a.at(0x1027ea18); a.load<1>(0,15,0x109); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027ec0c);
            primary_phase_357(a,state);
        } else {
            a.at(0x1027ea24); a.load<1>(0,15,0x10e); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027ec0c);
                primary_phase_357(a,state);
            } else {
                a.at(0x1027ea30);
                primary_phase_352(a,state);
            }
        }
    }
}
void primary_phase_350(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ea0c); a.value(0,0x0);
    primary_phase_351(a,state);
}
void primary_phase_349(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ea04); a.value(0,0x1); a.jump(0x1027ea10);
    primary_phase_351(a,state);
}
void primary_phase_348(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e9e8); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ea0c);
        primary_phase_350(a,state);
    } else {
        a.at(0x1027e9f4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ea0c);
            primary_phase_350(a,state);
        } else {
            a.at(0x1027ea04);
            primary_phase_349(a,state);
        }
    }
}
void primary_phase_347(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e9a4); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ec0c);
        primary_phase_357(a,state);
    } else {
        a.at(0x1027e9ac); a.floating_load(0,26,200); a.floating_compare(31,0); a.compare_greater_or_equal_bit();
        if (!a.equal()) {
            a.at(0x1027ec0c);
            primary_phase_357(a,state);
        } else {
            a.at(0x1027e9bc); a.load<1>(0,1,0xfd); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027ec0c);
                primary_phase_357(a,state);
            } else {
                a.at(0x1027e9cc); a.load<1>(0,15,0x27); a.compare(0,1);
                if (!a.equal()) {
                    a.at(0x1027e9e8);
                    primary_phase_348(a,state);
                } else {
                    a.at(0x1027e9d8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
                    if (a.at_least()) {
                        a.at(0x1027ea04);
                        primary_phase_349(a,state);
                    } else {
                        a.at(0x1027e9e8);
                        primary_phase_348(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_346(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e9a0); a.sign8(0,3);
    primary_phase_347(a,state);
}
void primary_phase_345(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e954); a.sign8(0,3); a.jump(0x1027e9a4);
    primary_phase_347(a,state);
}
void primary_phase_344(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e8dc); a.sign8(3,25); a.sign8(0,24); a.product(0,3,0); a.sign8(3,14); a.sign8(4,13);
    a.product(0,3,0); a.product(0,4,0); a.compare_registers(0,23);
    if (a.nonpositive()) {
        a.at(0x1027f0d0);
        primary_phase_370(a,state);
    } else {
        a.at(0x1027e900); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027e95c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027e970); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027e980); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x1027e98c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027e99c); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027e9a0);
            primary_phase_346(a,state);
        } else {
            a.at(0x1027e90c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027e920); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027e930); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x1027e940); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027e950); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027e954);
            primary_phase_345(a,state);
        }
    }
}
void primary_phase_343(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e8d8); a.move(23,0);
    primary_phase_344(a,state);
}
void primary_phase_342(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e750); a.floating_load(0,31,208,true); a.value(0,0x43300000); a.load(3,30,0x0);
    a.value(7,0x80010000); a.floating_product_single(1,0,31); a.floating_load(0,31,208,true); a.store(1,0xac,0);
    a.value(5,0x43300000); a.floating_product_single(0,0,31); a.value(6,0x10000); a.floating_integer(1,1);
    a.value(4,0x80010000); a.floating_integer(0,0); a.load<2>(8,3,0x0); a.load(0,27,0x0); a.offset(7,7,-0x7fff);
    a.floating_store(1,1,164); a.add(0,8,0); a.floating_load(3,26,128); a.multiply_high(7,7,0); a.load(3,1,0xa8);
    a.offset(6,6,-0x1); a.store(1,0xb4,5); a.offset(4,4,-0x7fff); a.exclusive_flags(3,3,0x80000000);
    a.store(1,0xb0,3); a.add(5,7,0); a.floating_load(2,26,0); a.shift_right(5,5,0xf); a.floating_store(0,1,196);
    a.mask(7,5,0x1,false,1); a.floating_load(0,1,172); a.add(5,5,7); a.product(5,5,6); a.floating_load(1,26,128);
    a.floating_subtract(0,0,3); a.load(3,1,0xc8); a.floating_difference(2,0,2,true); a.subtract(23,5,0);
    a.exclusive_flags(0,23,0x80000000); a.store(1,0xb8,0); a.multiply(0,3,0xa); a.floating_load(0,1,180);
    a.floating_subtract(0,0,1); a.multiply_high(3,4,0); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_integer(0,0); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.floating_store(0,1,188); a.load(23,1,0xc0); a.call(0x10226520); a.floating_load(0,31,208,true);
    a.value(4,0x66660000); a.offset(0,3,0x5); a.floating_product_single(0,0,31); a.offset(4,4,0x6667);
    a.multiply_high(0,4,0); a.floating_integer(0,0); a.floating_store(0,1,204); a.shift_right(0,0,0x2);
    a.mask(4,0,0x1,false,1); a.load(3,1,0xd0); a.add(0,0,4); a.add(23,23,0); a.offset(0,3,-0x1);
    a.compare_registers(23,0);
    if (a.positive()) {
        a.at(0x1027e868); a.floating_load(0,31,208,true); a.floating_product_single(0,0,31);
        a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8); a.offset(23,3,-0x1);
        a.jump(0x1027e8dc);
    }
    a.at(0x1027e8dc);
    primary_phase_344(a,state);
}
void primary_phase_341(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e708); a.floating_load(0,31,208,true); a.value(3,0x10000); a.offset(3,3,-0x1);
    a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027e750);
        primary_phase_342(a,state);
    } else {
        a.at(0x1027e72c); a.floating_load(0,31,208,true); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027e884); a.floating_load(0,31,208,true); a.load(3,30,0x0); a.floating_product_single(1,0,31);
            a.floating_load(0,31,208,true); a.load<2>(3,3,0x0); a.load(0,27,0x0);
            a.floating_product_single(0,0,31); a.floating_integer(1,1); a.add(5,3,0); a.floating_integer(0,0);
            a.floating_store(1,1,164); a.load(4,1,0xa8); a.floating_store(0,1,172); a.divide(3,5,4);
            a.load(0,1,0xb0); a.compare(0,0); a.product(0,3,4); a.subtract(23,0,5); a.negate(0,23);
            if (a.positive()) {
                a.at(0x1027e8d4); a.move(0,23);
            }
            a.at(0x1027e8d8);
            primary_phase_343(a,state);
        } else {
            a.at(0x1027e750);
            primary_phase_342(a,state);
        }
    }
}
void primary_phase_340(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e64c); a.floating_load(4,31,148,true); a.floating_product_single(1,30,30);
    a.floating_load(3,15,160,true); a.sign8(3,18); a.floating_load(2,31,168,true); a.load<1>(0,1,0xfe);
    a.floating_load(0,26,16);
    a.floating_product_difference(2,4,3,2); { const auto rounded=-double(float(float_value<8>(state.r<8>(16416)))); state.wr<8>(16416,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.sign8(0,0); a.subtract(4,3,0); a.floating_compare(31,0); a.floating_product_single(0,2,1);
    a.load<1>(0,1,0xfe); a.sign8(3,18); a.sign8(0,0); a.subtract(0,3,0); a.floating_integer(0,0);
    a.multiply(0,0,0x64); a.floating_store(0,1,164); a.subtract(20,0,20); a.load(0,1,0xa8); a.multiply(3,4,0x64);
    a.subtract(19,0,19); a.subtract(19,3,19); a.compare_greater_or_equal_bit();
    if (!a.equal()) {
        a.at(0x1027f0d0);
        primary_phase_370(a,state);
    } else {
        a.at(0x1027e6b4); a.floating_load(1,31,208,true); a.floating_load(0,31,0,true);
        a.floating_product_single(1,1,31); a.load<1>(13,15,0x10c); a.load<1>(14,15,0x10c); a.load<1>(25,15,0x10c);
        a.floating_compare(1,0); a.load<1>(24,15,0x10c);
        if (!a.equal()) {
            a.at(0x1027e6e0); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027e6f8); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027e708);
            primary_phase_341(a,state);
        } else {
            a.at(0x1027e6d8); a.value(23,0x0); a.jump(0x1027e8dc);
            primary_phase_344(a,state);
        }
    }
}
void primary_phase_339(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e638); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,3);
    if (a.equal()) {
        a.at(0x1027e648); a.offset(21,21,-0x7d0);
    }
    a.at(0x1027e64c);
    primary_phase_340(a,state);
}
void primary_phase_338(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e5dc); a.floating_load(0,26,24); a.floating_compare(31,0);
    if (a.less()) {
        a.at(0x1027e5e8); a.load<2>(4,1,0x58,true); a.value(0,0x43300000); a.load<2>(3,1,0x58,true);
        a.floating_load(0,26,184); a.product(3,4,3); a.load<2>(4,1,0x58,true); a.floating_subtract(0,0,31);
        a.store(1,0xa4,0); a.floating_load(2,26,128); a.floating_round_single(0,0); a.product(0,4,3);
        a.exclusive_flags(0,0,0x80000000); a.store(1,0xa8,0); a.floating_load(1,1,164);
        a.floating_subtract(1,1,2,true); a.floating_difference(0,1,0,true,true); a.floating_integer(0,0);
        a.floating_store(0,1,172); a.load(0,1,0xb0); a.subtract(21,0,21);
    }
    a.at(0x1027e638);
    primary_phase_339(a,state);
}
void primary_phase_337(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e5d8); a.subtract(21,13,21);
    primary_phase_338(a,state);
}
void primary_phase_336(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e5d4); a.move(13,0);
    primary_phase_337(a,state);
}
void primary_phase_335(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e4c4); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027e5a0); a.offset(13,14,-0x1); a.jump(0x1027e5d8);
    }
    a.at(0x1027e5d8);
    primary_phase_337(a,state);
}
void primary_phase_334(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e4a4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027e4c4);
        primary_phase_335(a,state);
    } else {
        a.at(0x1027e4b4); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027e5a8); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027e5d0); a.move(0,13);
            }
            a.at(0x1027e5d4);
            primary_phase_336(a,state);
        } else {
            a.at(0x1027e4c4);
            primary_phase_335(a,state);
        }
    }
}
void primary_phase_333(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e3e0); a.load<1>(4,15,0x27); a.value(0,0x1); a.floating_load(0,26,16); a.sign8(3,0); a.sign8(5,4);
    a.value(0,0xb); a.floating_sum(1,0,31); a.subtract(4,3,5); a.subtract(3,5,3); a.load<1>(5,15,0x105);
    a.complement_union(3,4,3); a.load<1>(6,15,0x106); a.shift_right(3,3,0x1f); a.floating_round_single(1,1);
    a.intersect(0,0,3,true); a.move(3,15); a.sign8(8,0); a.offset(10,1,0x5a); a.value(4,0x16); a.value(7,0x4);
    a.call(0x102818f0); a.sign8(0,3); a.compare(0,2);
    if (a.equal()) {
        a.at(0x1027e5dc);
        primary_phase_338(a,state);
    } else {
        a.at(0x1027e440); a.load<1>(0,15,0x2d); a.sign8(3,3); a.sign8(0,0); a.compare_registers(3,0);
        if (a.equal()) {
            a.at(0x1027e5dc);
            primary_phase_338(a,state);
        } else {
            a.at(0x1027e454); a.load<1>(0,1,0xff); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027e5dc);
                primary_phase_338(a,state);
            } else {
                a.at(0x1027e460); a.load<1>(0,15,0x2e); a.sign8(0,0); a.multiply(14,0,0x12c); a.compare(14,0);
                if (!a.equal()) {
                    a.at(0x1027e47c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027e494); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027e4a4);
                    primary_phase_334(a,state);
                } else {
                    a.at(0x1027e474); a.value(13,0x0); a.jump(0x1027e5d8);
                    primary_phase_337(a,state);
                }
            }
        }
    }
}
void primary_phase_332(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e3ac); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,2);
    if (a.less()) {
        a.at(0x1027e3e0);
        primary_phase_333(a,state);
    } else {
        a.at(0x1027e3c0); a.floating_load(0,26,808); a.floating_compare(31,0);
        if (a.nonpositive()) {
            a.at(0x1027e5dc);
            primary_phase_338(a,state);
        } else {
            a.at(0x1027e3cc); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,4);
            if (a.at_least()) {
                a.at(0x1027e5dc);
                primary_phase_338(a,state);
            } else {
                a.at(0x1027e3e0);
                primary_phase_333(a,state);
            }
        }
    }
}
void primary_phase_331(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e398); a.multiply(0,13,0x190); a.load<2>(3,15,0x2a,true); a.mask(3,3,0xfffffffe,false,1);
    a.add(21,0,21); a.add(21,3,21);
    primary_phase_332(a,state);
}
void primary_phase_330(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e394); a.move(13,0);
    primary_phase_331(a,state);
}
void primary_phase_329(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e284); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027e360); a.offset(13,14,-0x1); a.jump(0x1027e398);
    }
    a.at(0x1027e398);
    primary_phase_331(a,state);
}
void primary_phase_328(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e264); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027e284);
        primary_phase_329(a,state);
    } else {
        a.at(0x1027e274); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027e368); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027e390); a.move(0,13);
            }
            a.at(0x1027e394);
            primary_phase_330(a,state);
        } else {
            a.at(0x1027e284);
            primary_phase_329(a,state);
        }
    }
}
void primary_phase_327(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e1f0); a.move(3,15); a.call(0x10286390); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027e3ac);
        primary_phase_332(a,state);
    } else {
        a.at(0x1027e200); a.floating_load(0,26,200); a.floating_compare(31,0); a.compare_greater_or_equal_bit();
        if (!a.equal()) {
            a.at(0x1027e3ac);
            primary_phase_332(a,state);
        } else {
            a.at(0x1027e210); a.floating_load(0,26,856); a.floating_compare(31,0);
            if (a.at_least()) {
                a.at(0x1027e3ac);
                primary_phase_332(a,state);
            } else {
                a.at(0x1027e21c); a.floating_load(0,15,180,true); a.floating_integer(0,0);
                a.floating_store(0,1,164); a.load(14,1,0xa8); a.compare(14,0);
                if (!a.equal()) {
                    a.at(0x1027e23c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027e254); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027e264);
                    primary_phase_328(a,state);
                } else {
                    a.at(0x1027e234); a.value(13,0x0); a.jump(0x1027e398);
                    primary_phase_331(a,state);
                }
            }
        }
    }
}
void primary_phase_326(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e1e4); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027e1ec); a.offset(20,20,0xdac);
    }
    a.at(0x1027e1f0);
    primary_phase_327(a,state);
}
void primary_phase_325(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e1c4); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0x0); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60);
    primary_phase_326(a,state);
}
void primary_phase_324(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e16c); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0xb); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60); a.jump(0x1027e1e4);
    primary_phase_326(a,state);
}
void primary_phase_323(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e124); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027e1f0);
        primary_phase_327(a,state);
    } else {
        a.at(0x1027e12c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027e190); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027e1c4);
                primary_phase_325(a,state);
            } else {
                a.at(0x1027e19c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x1027e1c4);
                    primary_phase_325(a,state);
                } else {
                    a.at(0x1027e1ac); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x1027e1c4);
                        primary_phase_325(a,state);
                    } else {
                        a.at(0x1027e1bc); a.value(3,0x1); a.jump(0x1027e1e4);
                        primary_phase_326(a,state);
                    }
                }
            }
        } else {
            a.at(0x1027e138); a.load<1>(0,15,0x106); a.compare(0,11);
            if (!a.equal()) {
                a.at(0x1027e16c);
                primary_phase_324(a,state);
            } else {
                a.at(0x1027e144); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x1027e16c);
                    primary_phase_324(a,state);
                } else {
                    a.at(0x1027e154); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x1027e16c);
                        primary_phase_324(a,state);
                    } else {
                        a.at(0x1027e164); a.value(3,0x1); a.jump(0x1027e1e4);
                        primary_phase_326(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_322(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e120); a.value(0,0x0);
    primary_phase_323(a,state);
}
void primary_phase_321(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e118); a.value(0,0x1); a.jump(0x1027e124);
    primary_phase_323(a,state);
}
void primary_phase_320(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e0fc); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027e120);
        primary_phase_322(a,state);
    } else {
        a.at(0x1027e108); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027e120);
            primary_phase_322(a,state);
        } else {
            a.at(0x1027e118);
            primary_phase_321(a,state);
        }
    }
}
void primary_phase_319(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e0d0); a.move(3,15); a.call(0x10286400); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027e1f0);
        primary_phase_327(a,state);
    } else {
        a.at(0x1027e0e0); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027e0fc);
            primary_phase_320(a,state);
        } else {
            a.at(0x1027e0ec); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x1027e118);
                primary_phase_321(a,state);
            } else {
                a.at(0x1027e0fc);
                primary_phase_320(a,state);
            }
        }
    }
}
void primary_phase_318(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e088); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027e090); a.move(3,15); a.call(0x102864b0); a.value(5,0x0); a.value(0,0x4); a.intersect(6,4,5);
        a.value(4,0x0); a.intersect(5,3,0); a.value(0,0x0); a.combine(3,6,4,false,true);
        a.combine(0,5,0,false,true); a.combine(0,3,0,true);
        if (a.equal()) {
        } else {
            a.at(0x1027e0c0); a.load(3,1,0x68); a.offset(19,19,0x1f4); a.offset(0,3,-0xfa); a.store(1,0x68,0);
        }
    }
    a.at(0x1027e0d0);
    primary_phase_319(a,state);
}
void primary_phase_317(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e084); a.value(0,0x0);
    primary_phase_318(a,state);
}
void primary_phase_316(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e07c); a.value(0,0x1); a.jump(0x1027e088);
    primary_phase_318(a,state);
}
void primary_phase_315(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e060); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027e084);
        primary_phase_317(a,state);
    } else {
        a.at(0x1027e06c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027e084);
            primary_phase_317(a,state);
        } else {
            a.at(0x1027e07c);
            primary_phase_316(a,state);
        }
    }
}
void primary_phase_314(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e02c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027e0d0);
        primary_phase_319(a,state);
    } else {
        a.at(0x1027e034); a.move(3,15); a.call(0x1028abc0); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x1027e0d0);
            primary_phase_319(a,state);
        } else {
            a.at(0x1027e044); a.load<1>(0,15,0x27); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x1027e060);
                primary_phase_315(a,state);
            } else {
                a.at(0x1027e050); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x1027e07c);
                    primary_phase_316(a,state);
                } else {
                    a.at(0x1027e060);
                    primary_phase_315(a,state);
                }
            }
        }
    }
}
void primary_phase_313(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027e020); a.value(0,0x1); a.jump(0x1027e02c);
    primary_phase_314(a,state);
}
void primary_phase_312(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027df80); a.load<2>(0,15,0x2a,true); a.value(7,0x43300000); a.load<1>(4,15,0x11);
    a.value(3,0x43300000); a.multiply(5,0,0xa); a.load(6,1,0x68); a.load<1>(8,15,0x10c); a.load<1>(0,15,0x105);
    a.subtract_from(6,6,0xfa0); a.multiply(4,4,0x12c); a.sign8(8,8); a.store(1,0xa4,7); a.sign8(0,0);
    a.floating_load(1,15,228,true); a.add(4,5,4); a.add(4,6,4); a.exclusive_flags(5,8,0x80000000);
    a.exclusive_flags(4,4,0x80000000); a.store(1,0xa8,5); a.floating_load(0,15,152,true); a.compare(0,2);
    a.store(1,0xb0,4); a.floating_load(3,26,128); a.floating_product_single(4,1,0); a.store(1,0xac,3);
    a.floating_load(2,1,164); a.floating_load(1,26,128); a.floating_load(0,1,172); a.floating_subtract(2,2,3,true);
    a.floating_subtract(0,0,1,true); a.floating_product_sum(0,4,2,0,true); a.floating_integer(0,0);
    a.floating_store(0,1,180); a.load(0,1,0xb8); a.add(19,19,0);
    if (a.less()) {
        a.at(0x1027e020);
        primary_phase_313(a,state);
    } else {
        a.at(0x1027e010); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
        if (a.nonpositive()) {
            a.at(0x1027e028); a.value(0,0x0);
            primary_phase_314(a,state);
        } else {
            a.at(0x1027e020);
            primary_phase_313(a,state);
        }
    }
}
void primary_phase_311(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027df5c); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.floating_load(1,26,128); a.store(1,0xa4,0); a.floating_load(0,1,164); a.floating_subtract(0,0,1,true);
    a.floating_compare(0,31);
    if (a.at_least()) {
        a.at(0x1027e0d0);
        primary_phase_319(a,state);
    } else {
        a.at(0x1027df80);
        primary_phase_312(a,state);
    }
}
void primary_phase_310(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027df58); a.move(14,0);
    primary_phase_311(a,state);
}
void primary_phase_309(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027de0c); a.sign8(0,13); a.value(3,0x43300000); a.mask(4,0,0xfffffffe,false,1); a.load(8,30,0x0);
    a.offset(0,4,0x14); a.value(7,0x80010000); a.exclusive_flags(0,0,0x80000000); a.value(5,0x43300000);
    a.store(1,0xa8,0); a.value(6,0x10000); a.value(4,0x80010000); a.floating_load(2,26,128); a.store(1,0xa4,3);
    a.sign8(0,13); a.mask(3,0,0xfffffffe,false,1); a.floating_load(0,26,0); a.floating_load(1,1,164);
    a.offset(0,3,0x14); a.load<2>(8,8,0x0); a.offset(7,7,-0x7fff); a.floating_subtract(1,1,2); a.load(3,27,0x0);
    a.store(1,0xac,5); a.offset(6,6,-0x1); a.add(8,8,3); a.offset(4,4,-0x7fff); a.floating_difference(2,1,0,true);
    a.multiply_high(3,7,8); a.floating_load(1,26,128); a.add(3,3,8); a.shift_right(3,3,0xf);
    a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,6); a.subtract(14,3,8);
    a.exclusive_flags(3,14,0x80000000); a.store(1,0xb0,3); a.multiply(0,0,0xa); a.floating_load(0,1,172);
    a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
    a.floating_store(0,1,180); a.load(14,1,0xb8); a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5);
    a.offset(4,4,0x6667); a.sign8(0,13); a.multiply_high(4,4,3); a.mask(3,0,0xfffffffe,false,1);
    a.offset(0,3,0x13); a.shift_right(3,4,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027df08); a.sign8(0,13); a.mask(3,0,0xfffffffe,false,1); a.offset(14,3,0x13); a.jump(0x1027df5c);
    }
    a.at(0x1027df5c);
    primary_phase_311(a,state);
}
void primary_phase_308(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ddd4); a.sign8(0,13); a.value(3,0x10000); a.mask(4,0,0xfffffffe,false,1); a.offset(0,4,0x14);
    a.offset(3,3,-0x1); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027de0c);
        primary_phase_309(a,state);
    } else {
        a.at(0x1027ddf0); a.sign8(0,13); a.value(3,0xffff0000); a.mask(4,0,0xfffffffe,false,1); a.offset(0,4,0x14);
        a.offset(3,3,0x1); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027df18); a.load(3,30,0x0); a.sign8(0,13); a.mask(4,0,0xfffffffe,false,1); a.sign8(0,13);
            a.load<2>(6,3,0x0); a.mask(3,0,0xfffffffe,false,1); a.load(5,27,0x0); a.offset(4,4,0x14);
            a.offset_record(0,3,0x14); a.add(3,6,5); a.divide(0,3,4); a.product(0,0,4); a.subtract(14,0,3);
            a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027df54); a.move(0,14);
            }
            a.at(0x1027df58);
            primary_phase_310(a,state);
        } else {
            a.at(0x1027de0c);
            primary_phase_309(a,state);
        }
    }
}
void primary_phase_307(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd88); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027df80);
        primary_phase_312(a,state);
    } else {
        a.at(0x1027dd90); a.load<1>(13,15,0x109); a.sign8(0,13); a.mask(3,0,0xfffffffe,false,1);
        a.offset_record(0,3,0x14);
        if (!a.equal()) {
            a.at(0x1027ddac); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027ddc4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027ddd4);
            primary_phase_308(a,state);
        } else {
            a.at(0x1027dda4); a.value(14,0x0); a.jump(0x1027df5c);
            primary_phase_311(a,state);
        }
    }
}
void primary_phase_306(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd84); a.value(0,0x0);
    primary_phase_307(a,state);
}
void primary_phase_305(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd7c); a.value(0,0x1); a.jump(0x1027dd88);
    primary_phase_307(a,state);
}
void primary_phase_304(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd60); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027dd84);
        primary_phase_306(a,state);
    } else {
        a.at(0x1027dd6c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027dd84);
            primary_phase_306(a,state);
        } else {
            a.at(0x1027dd7c);
            primary_phase_305(a,state);
        }
    }
}
void primary_phase_303(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd10); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027e0d0);
        primary_phase_319(a,state);
    } else {
        a.at(0x1027dd18); a.load<1>(0,1,0xfd); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027e0d0);
            primary_phase_319(a,state);
        } else {
            a.at(0x1027dd28); a.floating_load(0,26,200); a.floating_compare(31,0);
            a.compare_greater_or_equal_bit();
            if (!a.equal()) {
                a.at(0x1027e0d0);
                primary_phase_319(a,state);
            } else {
                a.at(0x1027dd38); a.load<2>(0,1,0x58,true); a.compare(0,0);
                if (a.equal()) {
                    a.at(0x1027df80);
                    primary_phase_312(a,state);
                } else {
                    a.at(0x1027dd44); a.load<1>(0,15,0x27); a.compare(0,1);
                    if (!a.equal()) {
                        a.at(0x1027dd60);
                        primary_phase_304(a,state);
                    } else {
                        a.at(0x1027dd50); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
                        if (a.at_least()) {
                            a.at(0x1027dd7c);
                            primary_phase_305(a,state);
                        } else {
                            a.at(0x1027dd60);
                            primary_phase_304(a,state);
                        }
                    }
                }
            }
        }
    }
}
void primary_phase_302(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dd0c); a.sign8(0,3);
    primary_phase_303(a,state);
}
void primary_phase_301(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dcc0); a.sign8(0,3); a.jump(0x1027dd10);
    primary_phase_303(a,state);
}
void primary_phase_300(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dc50); a.sign8(0,16); a.compare(0,4);
    if (a.equal()) {
        a.at(0x1027e3ac);
        primary_phase_332(a,state);
    } else {
        a.at(0x1027dc5c); a.move(3,15); a.call(0x10286320); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x1027e0d0);
            primary_phase_319(a,state);
        } else {
            a.at(0x1027dc6c); a.load<1>(0,15,0x27); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x1027dcc8); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x1027dcdc); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027dcec); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                        if (a.less()) {
                        } else {
                            a.at(0x1027dcf8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                            if (a.positive()) {
                            } else {
                                a.at(0x1027dd08); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x1027dd0c);
                primary_phase_302(a,state);
            } else {
                a.at(0x1027dc78); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x1027dc8c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027dc9c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                        if (a.less()) {
                        } else {
                            a.at(0x1027dcac); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                            if (a.positive()) {
                            } else {
                                a.at(0x1027dcbc); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x1027dcc0);
                primary_phase_301(a,state);
            }
        }
    }
}
void primary_phase_299(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dbf8); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027dc00); a.load<1>(3,15,0x10c); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
        a.floating_load(1,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(4,31,132,true);
        a.store(1,0xa8,0); a.floating_load(3,31,240,true); a.floating_load(0,1,164);
        a.floating_load(2,15,156,true);
        a.floating_product_difference(3,4,31,3); { const auto rounded=-double(float(float_value<8>(state.r<8>(16432)))); state.wr<8>(16432,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_subtract(0,0,1,true); a.floating_sum_single(0,2,0); a.floating_product_single(0,3,0);
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(21,21,0); a.jump(0x1027e3ac);
    }
    a.at(0x1027e3ac);
    primary_phase_332(a,state);
}
void primary_phase_298(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dbf4); a.sign8(0,3);
    primary_phase_299(a,state);
}
void primary_phase_297(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027dba8); a.sign8(0,3); a.jump(0x1027dbf8);
    primary_phase_299(a,state);
}
void primary_phase_296(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027db44); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027dbb0); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027dbc4); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027dbd4); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027dbe0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027dbf0); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027dbf4);
        primary_phase_298(a,state);
    } else {
        a.at(0x1027db60); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027db74); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027db84); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027db94); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027dba4); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027dba8);
        primary_phase_297(a,state);
    }
}
void primary_phase_295(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027db28); a.load<2>(3,15,0x2a,true); a.multiply(0,13,0x190); a.mask(3,3,0xfffffffe,false,1);
    a.add(0,3,0); a.add(21,0,21); a.offset(21,21,0x9c4); a.jump(0x1027e3ac);
    primary_phase_332(a,state);
}
void primary_phase_294(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027db24); a.move(13,0);
    primary_phase_295(a,state);
}
void primary_phase_293(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027da14); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027daf0); a.offset(13,14,-0x1); a.jump(0x1027db28);
    }
    a.at(0x1027db28);
    primary_phase_295(a,state);
}
void primary_phase_292(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d9f4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027da14);
        primary_phase_293(a,state);
    } else {
        a.at(0x1027da04); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027daf8); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027db20); a.move(0,13);
            }
            a.at(0x1027db24);
            primary_phase_294(a,state);
        } else {
            a.at(0x1027da14);
            primary_phase_293(a,state);
        }
    }
}
void primary_phase_291(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d998); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027db44);
        primary_phase_296(a,state);
    } else {
        a.at(0x1027d9a0); a.load<1>(0,15,0x10c); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x1027e3ac);
            primary_phase_332(a,state);
        } else {
            a.at(0x1027d9ac); a.floating_load(0,15,180,true); a.floating_integer(0,0); a.floating_store(0,1,164);
            a.load(14,1,0xa8); a.compare(14,0);
            if (!a.equal()) {
                a.at(0x1027d9cc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027d9e4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027d9f4);
                primary_phase_292(a,state);
            } else {
                a.at(0x1027d9c4); a.value(13,0x0); a.jump(0x1027db28);
                primary_phase_295(a,state);
            }
        }
    }
}
void primary_phase_290(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d98c); a.value(0,0x1); a.jump(0x1027d998);
    primary_phase_291(a,state);
}
void primary_phase_289(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d950); a.floating_load(0,26,200); a.floating_compare(31,0); a.compare_greater_or_equal_bit();
    if (!a.equal()) {
        a.at(0x1027db44);
        primary_phase_296(a,state);
    } else {
        a.at(0x1027d960); a.floating_load(0,26,856); a.floating_compare(31,0);
        if (a.at_least()) {
            a.at(0x1027db44);
            primary_phase_296(a,state);
        } else {
            a.at(0x1027d96c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
            if (a.less()) {
                a.at(0x1027d98c);
                primary_phase_290(a,state);
            } else {
                a.at(0x1027d97c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.nonpositive()) {
                    a.at(0x1027d994); a.value(0,0x0);
                    primary_phase_291(a,state);
                } else {
                    a.at(0x1027d98c);
                    primary_phase_290(a,state);
                }
            }
        }
    }
}
void primary_phase_288(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d914); a.sign8(3,0); a.value(0,0x43300000); a.multiply(3,3,0x3e8); a.store(1,0xa4,0);
    a.floating_load(2,26,128); a.subtract_from(0,3,0x3e8); a.exclusive_flags(0,0,0x80000000); a.store(1,0xa8,0);
    a.floating_load(0,1,164); a.floating_subtract(0,0,2); a.floating_sum(0,1,0); a.floating_integer(0,0);
    a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(19,19,0);
    primary_phase_289(a,state);
}
void primary_phase_287(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d910); a.sign8(0,3);
    primary_phase_288(a,state);
}
void primary_phase_286(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d8c4); a.sign8(0,3); a.jump(0x1027d914);
    primary_phase_288(a,state);
}
void primary_phase_285(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d85c); a.load<1>(13,15,0x27); a.floating_load(1,15,152,true); a.floating_load(2,26,8);
    a.call(0x10780fc0); a.load(2,1,0x14); a.sign8(0,13); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027d8cc); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027d8e0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027d8f0); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027d8fc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027d90c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027d910);
        primary_phase_287(a,state);
    } else {
        a.at(0x1027d87c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027d890); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027d8a0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027d8b0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027d8c0); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027d8c4);
        primary_phase_286(a,state);
    }
}
void primary_phase_284(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d83c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027d85c);
        primary_phase_285(a,state);
    } else {
        a.at(0x1027d844); a.load<1>(0,15,0x109); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027d950);
            primary_phase_289(a,state);
        } else {
            a.at(0x1027d850); a.load<1>(0,15,0x10e); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027d950);
                primary_phase_289(a,state);
            } else {
                a.at(0x1027d85c);
                primary_phase_285(a,state);
            }
        }
    }
}
void primary_phase_283(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d838); a.value(0,0x0);
    primary_phase_284(a,state);
}
void primary_phase_282(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d830); a.value(0,0x1); a.jump(0x1027d83c);
    primary_phase_284(a,state);
}
void primary_phase_281(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d814); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027d838);
        primary_phase_283(a,state);
    } else {
        a.at(0x1027d820); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027d838);
            primary_phase_283(a,state);
        } else {
            a.at(0x1027d830);
            primary_phase_282(a,state);
        }
    }
}
void primary_phase_280(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d7f8); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027d814);
        primary_phase_281(a,state);
    } else {
        a.at(0x1027d804); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027d830);
            primary_phase_282(a,state);
        } else {
            a.at(0x1027d814);
            primary_phase_281(a,state);
        }
    }
}
void primary_phase_279(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d7d0); a.load(3,30,0x0); a.load(0,27,0x0); a.load<2>(3,3,0x0); a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30); a.mask(3,3,0x1,false,1); a.subtract(0,3,0); a.mask(0,0,0xffffffff,false,2);
    a.add_record(0,0,3);
    if (!a.equal()) {
        a.at(0x1027dc50);
        primary_phase_300(a,state);
    } else {
        a.at(0x1027d7f8);
        primary_phase_280(a,state);
    }
}
void primary_phase_278(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d78c); a.compare(13,0);
    if (!a.equal()) {
        a.at(0x1027dc50);
        primary_phase_300(a,state);
    } else {
        a.at(0x1027d794); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.mask(0,3,0x40,true,0);
        if (!a.equal()) {
            a.at(0x1027d7f8);
            primary_phase_280(a,state);
        } else {
            a.at(0x1027d7a8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027d7c0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027d7d0);
            primary_phase_279(a,state);
        }
    }
}
void primary_phase_277(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d788); a.move(13,0);
    primary_phase_278(a,state);
}
void primary_phase_276(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d678); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027d754); a.offset(13,14,-0x1); a.jump(0x1027d78c);
    }
    a.at(0x1027d78c);
    primary_phase_278(a,state);
}
void primary_phase_275(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d658); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027d678);
        primary_phase_276(a,state);
    } else {
        a.at(0x1027d668); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027d75c); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027d784); a.move(0,13);
            }
            a.at(0x1027d788);
            primary_phase_277(a,state);
        } else {
            a.at(0x1027d678);
            primary_phase_276(a,state);
        }
    }
}
void primary_phase_274(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d610); a.floating_load(0,15,228,true); a.floating_integer(0,0); a.floating_store(0,1,164);
    a.load(14,1,0xa8); a.compare(14,0);
    if (!a.equal()) {
        a.at(0x1027d630); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027d648); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027d658);
        primary_phase_275(a,state);
    } else {
        a.at(0x1027d628); a.value(13,0x0); a.jump(0x1027d78c);
        primary_phase_278(a,state);
    }
}
void primary_phase_273(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d5c8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027d5d0); a.load<1>(3,15,0x10c); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
        a.floating_load(1,26,128); a.multiply(0,0,0x4b); a.floating_load(2,31,56,true);
        a.exclusive_flags(0,0,0x80000000); a.store(1,0xa8,0); a.floating_load(0,1,164);
        a.floating_subtract(0,0,1,true);
        a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.subtract(21,0,21);
    }
    a.at(0x1027d610);
    primary_phase_274(a,state);
}
void primary_phase_272(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d5c4); a.value(0,0x0);
    primary_phase_273(a,state);
}
void primary_phase_271(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d5bc); a.value(0,0x1); a.jump(0x1027d5c8);
    primary_phase_273(a,state);
}
void primary_phase_270(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d5a0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027d5c4);
        primary_phase_272(a,state);
    } else {
        a.at(0x1027d5ac); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027d5c4);
            primary_phase_272(a,state);
        } else {
            a.at(0x1027d5bc);
            primary_phase_271(a,state);
        }
    }
}
void primary_phase_269(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d584); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027d5a0);
        primary_phase_270(a,state);
    } else {
        a.at(0x1027d590); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027d5bc);
            primary_phase_271(a,state);
        } else {
            a.at(0x1027d5a0);
            primary_phase_270(a,state);
        }
    }
}
void primary_phase_268(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d524); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027d584);
        primary_phase_269(a,state);
    } else {
        a.at(0x1027d52c); a.load<1>(3,15,0x10c); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
        a.floating_load(1,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(2,15,156,true);
        a.store(1,0xa8,0); a.floating_load(4,31,132,true); a.floating_load(0,1,164);
        a.floating_load(3,31,240,true); a.floating_subtract(1,0,1,true); a.floating_load(0,15,228,true);
        a.floating_product_difference(3,4,31,3); { const auto rounded=-double(float(float_value<8>(state.r<8>(16432)))); state.wr<8>(16432,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_sum_single(1,2,1); a.floating_subtract(0,1,0,true); a.floating_product_single(0,3,0);
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(21,21,0); a.jump(0x1027d610);
        primary_phase_274(a,state);
    }
}
void primary_phase_267(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d518); a.value(0,0x1); a.jump(0x1027d524);
    primary_phase_268(a,state);
}
void primary_phase_266(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d4f0); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027d584);
        primary_phase_269(a,state);
    } else {
        a.at(0x1027d4f8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x1027d518);
            primary_phase_267(a,state);
        } else {
            a.at(0x1027d508); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x1027d520); a.value(0,0x0);
                primary_phase_268(a,state);
            } else {
                a.at(0x1027d518);
                primary_phase_267(a,state);
            }
        }
    }
}
void primary_phase_265(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d4ec); a.sign8(0,3);
    primary_phase_266(a,state);
}
void primary_phase_264(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d4a0); a.sign8(0,3); a.jump(0x1027d4f0);
    primary_phase_266(a,state);
}
void primary_phase_263(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d43c); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027d4a8); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027d4bc); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027d4cc); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027d4d8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027d4e8); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027d4ec);
        primary_phase_265(a,state);
    } else {
        a.at(0x1027d458); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027d46c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027d47c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027d48c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027d49c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027d4a0);
        primary_phase_264(a,state);
    }
}
void primary_phase_262(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d428); a.sign8(0,13); a.multiply(0,0,0x145); a.subtract(0,14,0); a.add(21,21,0); a.jump(0x1027d610);
    primary_phase_274(a,state);
}
void primary_phase_261(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d424); a.move(14,0);
    primary_phase_262(a,state);
}
void primary_phase_260(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d134); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(0,3,0x1); a.value(4,0x43300000);
    a.floating_sum_single(1,0,31); a.exclusive_flags(0,0,0x80000000); a.store(1,0xa4,4); a.value(6,0x43300000);
    a.load(9,30,0x0); a.value(8,0x80010000); a.floating_product_single(1,1,31); a.store(1,0xa8,0); a.sign8(3,17);
    a.floating_load(0,31,52,true); a.offset(0,3,0x1); a.store(1,0xb4,6); a.floating_product_single(1,31,1);
    a.floating_load(3,26,128); a.floating_load(2,1,164); a.exclusive_flags(0,0,0x80000000); a.value(3,0x43300000);
    a.floating_sum_single(0,0,31); a.floating_subtract(2,2,3,true); a.value(7,0x10000);
    a.floating_product_single(1,31,1); a.floating_load(3,31,356,true); a.floating_product_single(0,0,31);
    a.value(5,0x43300000); a.floating_product_single(1,2,1); a.value(4,0x80010000);
    a.floating_product_single(0,31,0); a.load<2>(9,9,0x0); a.load(6,27,0x0); a.offset(8,8,-0x7fff);
    a.floating_product_single(1,3,1); a.store(1,0xd0,0); a.add(0,9,6); a.floating_load(7,26,128);
    a.store(1,0xcc,3); a.multiply_high(8,8,0); a.floating_integer(1,1); a.floating_load(2,26,128);
    a.floating_load(5,26,0); a.floating_product_single(0,31,0); a.offset(7,7,-0x1); a.floating_store(1,1,172);
    a.offset(4,4,-0x7fff); a.floating_load(1,1,204); a.load(6,1,0xb0); a.floating_subtract(1,1,2,true);
    a.floating_load(3,31,356,true); a.exclusive_flags(3,6,0x80000000); a.store(1,0xbc,5);
    a.floating_load(4,26,128); a.store(1,0xb8,3); a.add(3,8,0); a.floating_product_single(0,1,0);
    a.floating_load(6,1,180); a.shift_right(3,3,0xf); a.mask(6,3,0x1,false,1); a.floating_subtract(2,6,7);
    a.add(3,3,6); a.product(3,3,7); a.floating_product_single(0,3,0); a.floating_difference(2,2,5,true);
    a.subtract(14,3,0); a.exclusive_flags(0,14,0x80000000); a.store(1,0xc0,0); a.floating_load(1,1,188);
    a.floating_integer(0,0); a.floating_subtract(1,1,4); a.floating_store(0,1,212); a.floating_product(1,2,1);
    a.load(0,1,0xd8); a.multiply(0,0,0xa); a.floating_integer(0,1); a.multiply_high(3,4,0);
    a.floating_store(0,1,196); a.load(14,1,0xc8); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.add(3,0,3); a.call(0x10226520); a.floating_load(0,31,52,true); a.sign8(4,17); a.offset(4,4,0x1);
    a.value(0,0x43300000); a.floating_sum_single(0,0,31); a.exclusive_flags(4,4,0x80000000); a.value(5,0x66660000);
    a.store(1,0xe0,4); a.floating_load(2,26,128); a.offset(3,3,0x5); a.floating_product_single(0,0,31);
    a.store(1,0xdc,0); a.offset(5,5,0x6667); a.floating_load(3,31,356,true); a.floating_load(1,1,220);
    a.multiply_high(0,5,3); a.floating_product_single(0,31,0); a.floating_subtract(1,1,2,true);
    a.floating_product_single(0,31,0); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3);
    a.floating_product_single(0,1,0); a.add(14,14,0); a.floating_product_single(0,3,0); a.floating_integer(0,0);
    a.floating_store(0,1,228); a.load(3,1,0xe8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027d300); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
        a.floating_sum_single(0,0,31); a.exclusive_flags(3,3,0x80000000); a.store(1,0xf0,3);
        a.floating_load(2,26,128); a.floating_product_single(0,0,31); a.store(1,0xec,0);
        a.floating_load(3,31,356,true); a.floating_load(1,1,236); a.floating_product_single(0,31,0);
        a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
        a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,244); a.load(3,1,0xf8);
        a.offset(14,3,-0x1); a.jump(0x1027d428);
    }
    a.at(0x1027d428);
    primary_phase_262(a,state);
}
void primary_phase_259(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027d074); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
    a.floating_sum_single(0,0,31); a.exclusive_flags(4,3,0x80000000); a.value(3,0x10000); a.store(1,0xa8,4);
    a.floating_load(2,26,128); a.offset(3,3,-0x1); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
    a.floating_load(3,31,356,true); a.floating_load(1,1,164); a.floating_product_single(0,31,0);
    a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
    a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027d134);
        primary_phase_260(a,state);
    } else {
        a.at(0x1027d0d4); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
        a.floating_sum_single(0,0,31); a.exclusive_flags(4,3,0x80000000); a.value(3,0xffff0000); a.store(1,0xa8,4);
        a.floating_load(2,26,128); a.offset(3,3,0x1); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
        a.floating_load(3,31,356,true); a.floating_load(1,1,164); a.floating_product_single(0,31,0);
        a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
        a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027d358); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(0,3,0x1);
            a.value(4,0x43300000); a.floating_sum_single(1,0,31); a.exclusive_flags(5,0,0x80000000);
            a.load(6,30,0x0); a.sign8(3,17); a.floating_load(0,31,52,true); a.offset(0,3,0x1);
            a.floating_product_single(1,1,31); a.exclusive_flags(3,0,0x80000000); a.store(1,0xa8,5);
            a.floating_sum_single(0,0,31); a.value(0,0x43300000); a.floating_load(3,26,128); a.store(1,0xa4,4);
            a.floating_product_single(1,31,1); a.floating_product_single(0,0,31); a.floating_load(4,31,356,true);
            a.floating_load(2,1,164); a.floating_product_single(1,31,1); a.load<2>(5,6,0x0);
            a.floating_subtract(2,2,3,true); a.load(4,27,0x0); a.floating_product_single(0,31,0);
            a.store(1,0xb8,3); a.floating_product_single(1,2,1); a.add(5,5,4); a.store(1,0xb4,0);
            a.floating_product_single(0,31,0); a.floating_load(2,26,128); a.floating_product_single(4,4,1);
            a.floating_load(1,1,180); a.floating_load(3,31,356,true); a.floating_integer(4,4);
            a.floating_subtract(1,1,2,true); a.floating_store(4,1,172); a.floating_product_single(0,1,0);
            a.load(4,1,0xb0); a.divide(3,5,4); a.floating_product_single(0,3,0); a.floating_integer(0,0);
            a.floating_store(0,1,188); a.load(0,1,0xc0); a.compare(0,0); a.product(0,3,4); a.subtract(14,0,5);
            a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027d420); a.move(0,14);
            }
            a.at(0x1027d424);
            primary_phase_261(a,state);
        } else {
            a.at(0x1027d134);
            primary_phase_260(a,state);
        }
    }
}
void primary_phase_258(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cfdc); a.sign8(0,13); a.multiply(0,0,0x1f4); a.subtract(0,14,0); a.add(21,21,0); a.jump(0x1027d610);
    primary_phase_274(a,state);
}
void primary_phase_257(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cfd8); a.move(14,0);
    primary_phase_258(a,state);
}
void primary_phase_256(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cce8); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(0,3,0x1); a.value(4,0x43300000);
    a.floating_sum_single(1,0,31); a.exclusive_flags(0,0,0x80000000); a.store(1,0xa4,4); a.value(6,0x43300000);
    a.load(9,30,0x0); a.value(8,0x80010000); a.floating_product_single(1,1,31); a.store(1,0xa8,0); a.sign8(3,17);
    a.floating_load(0,31,52,true); a.offset(0,3,0x1); a.store(1,0xb4,6); a.floating_product_single(1,31,1);
    a.floating_load(3,26,128); a.floating_load(2,1,164); a.exclusive_flags(0,0,0x80000000); a.value(3,0x43300000);
    a.floating_sum_single(0,0,31); a.floating_subtract(2,2,3,true); a.value(7,0x10000);
    a.floating_product_single(1,31,1); a.floating_load(3,31,356,true); a.floating_product_single(0,0,31);
    a.value(5,0x43300000); a.floating_product_single(1,2,1); a.value(4,0x80010000);
    a.floating_product_single(0,31,0); a.load<2>(9,9,0x0); a.load(6,27,0x0); a.offset(8,8,-0x7fff);
    a.floating_product_single(1,3,1); a.store(1,0xd0,0); a.add(0,9,6); a.floating_load(7,26,128);
    a.store(1,0xcc,3); a.multiply_high(8,8,0); a.floating_integer(1,1); a.floating_load(2,26,128);
    a.floating_load(5,26,0); a.floating_product_single(0,31,0); a.offset(7,7,-0x1); a.floating_store(1,1,172);
    a.offset(4,4,-0x7fff); a.floating_load(1,1,204); a.load(6,1,0xb0); a.floating_subtract(1,1,2,true);
    a.floating_load(3,31,356,true); a.exclusive_flags(3,6,0x80000000); a.store(1,0xbc,5);
    a.floating_load(4,26,128); a.store(1,0xb8,3); a.add(3,8,0); a.floating_product_single(0,1,0);
    a.floating_load(6,1,180); a.shift_right(3,3,0xf); a.mask(6,3,0x1,false,1); a.floating_subtract(2,6,7);
    a.add(3,3,6); a.product(3,3,7); a.floating_product_single(0,3,0); a.floating_difference(2,2,5,true);
    a.subtract(14,3,0); a.exclusive_flags(0,14,0x80000000); a.store(1,0xc0,0); a.floating_load(1,1,188);
    a.floating_integer(0,0); a.floating_subtract(1,1,4); a.floating_store(0,1,212); a.floating_product(1,2,1);
    a.load(0,1,0xd8); a.multiply(0,0,0xa); a.floating_integer(0,1); a.multiply_high(3,4,0);
    a.floating_store(0,1,196); a.load(14,1,0xc8); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.add(3,0,3); a.call(0x10226520); a.floating_load(0,31,52,true); a.sign8(4,17); a.offset(4,4,0x1);
    a.value(0,0x43300000); a.floating_sum_single(0,0,31); a.exclusive_flags(4,4,0x80000000); a.value(5,0x66660000);
    a.store(1,0xe0,4); a.floating_load(2,26,128); a.offset(3,3,0x5); a.floating_product_single(0,0,31);
    a.store(1,0xdc,0); a.offset(5,5,0x6667); a.floating_load(3,31,356,true); a.floating_load(1,1,220);
    a.multiply_high(0,5,3); a.floating_product_single(0,31,0); a.floating_subtract(1,1,2,true);
    a.floating_product_single(0,31,0); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3);
    a.floating_product_single(0,1,0); a.add(14,14,0); a.floating_product_single(0,3,0); a.floating_integer(0,0);
    a.floating_store(0,1,228); a.load(3,1,0xe8); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027ceb4); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
        a.floating_sum_single(0,0,31); a.exclusive_flags(3,3,0x80000000); a.store(1,0xf0,3);
        a.floating_load(2,26,128); a.floating_product_single(0,0,31); a.store(1,0xec,0);
        a.floating_load(3,31,356,true); a.floating_load(1,1,236); a.floating_product_single(0,31,0);
        a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
        a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,244); a.load(3,1,0xf8);
        a.offset(14,3,-0x1); a.jump(0x1027cfdc);
    }
    a.at(0x1027cfdc);
    primary_phase_258(a,state);
}
void primary_phase_255(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cc28); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
    a.floating_sum_single(0,0,31); a.exclusive_flags(4,3,0x80000000); a.value(3,0x10000); a.store(1,0xa8,4);
    a.floating_load(2,26,128); a.offset(3,3,-0x1); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
    a.floating_load(3,31,356,true); a.floating_load(1,1,164); a.floating_product_single(0,31,0);
    a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
    a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027cce8);
        primary_phase_256(a,state);
    } else {
        a.at(0x1027cc88); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1); a.value(0,0x43300000);
        a.floating_sum_single(0,0,31); a.exclusive_flags(4,3,0x80000000); a.value(3,0xffff0000); a.store(1,0xa8,4);
        a.floating_load(2,26,128); a.offset(3,3,0x1); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
        a.floating_load(3,31,356,true); a.floating_load(1,1,164); a.floating_product_single(0,31,0);
        a.floating_subtract(1,1,2,true); a.floating_product_single(0,31,0); a.floating_product_single(0,1,0);
        a.floating_product_single(0,3,0); a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027cf0c); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(0,3,0x1);
            a.value(4,0x43300000); a.floating_sum_single(1,0,31); a.exclusive_flags(5,0,0x80000000);
            a.load(6,30,0x0); a.sign8(3,17); a.floating_load(0,31,52,true); a.offset(0,3,0x1);
            a.floating_product_single(1,1,31); a.exclusive_flags(3,0,0x80000000); a.store(1,0xa8,5);
            a.floating_sum_single(0,0,31); a.value(0,0x43300000); a.floating_load(3,26,128); a.store(1,0xa4,4);
            a.floating_product_single(1,31,1); a.floating_product_single(0,0,31); a.floating_load(4,31,356,true);
            a.floating_load(2,1,164); a.floating_product_single(1,31,1); a.load<2>(5,6,0x0);
            a.floating_subtract(2,2,3,true); a.load(4,27,0x0); a.floating_product_single(0,31,0);
            a.store(1,0xb8,3); a.floating_product_single(1,2,1); a.add(5,5,4); a.store(1,0xb4,0);
            a.floating_product_single(0,31,0); a.floating_load(2,26,128); a.floating_product_single(4,4,1);
            a.floating_load(1,1,180); a.floating_load(3,31,356,true); a.floating_integer(4,4);
            a.floating_subtract(1,1,2,true); a.floating_store(4,1,172); a.floating_product_single(0,1,0);
            a.load(4,1,0xb0); a.divide(3,5,4); a.floating_product_single(0,3,0); a.floating_integer(0,0);
            a.floating_store(0,1,188); a.load(0,1,0xc0); a.compare(0,0); a.product(0,3,4); a.subtract(14,0,5);
            a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027cfd4); a.move(0,14);
            }
            a.at(0x1027cfd8);
            primary_phase_257(a,state);
        } else {
            a.at(0x1027cce8);
            primary_phase_256(a,state);
        }
    }
}
void primary_phase_254(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cb78); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027d43c);
        primary_phase_263(a,state);
    } else {
        a.at(0x1027cb80); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
        if (a.equal()) {
            a.at(0x1027d43c);
            primary_phase_263(a,state);
        } else {
            a.at(0x1027cb90); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.mask(0,3,0x40,true,0);
            if (a.equal()) {
                a.at(0x1027cff0); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1);
                a.value(0,0x43300000); a.floating_sum_single(0,0,31); a.exclusive_flags(3,3,0x80000000);
                a.store(1,0xa8,3); a.floating_load(3,26,128); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
                a.floating_load(4,31,356,true); a.floating_load(2,1,164); a.floating_product_single(1,31,0);
                a.floating_load(0,31,0,true); a.floating_subtract(2,2,3,true); a.load<1>(13,15,0x10c);
                a.floating_product_single(1,31,1); a.floating_product_single(1,2,1);
                a.floating_product_single(1,4,1); a.floating_compare(1,0);
                if (!a.equal()) {
                    a.at(0x1027d04c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027d064); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027d074);
                    primary_phase_259(a,state);
                } else {
                    a.at(0x1027d044); a.value(14,0x0); a.jump(0x1027d428);
                    primary_phase_262(a,state);
                }
            } else {
                a.at(0x1027cba4); a.floating_load(0,31,52,true); a.sign8(3,17); a.offset(3,3,0x1);
                a.value(0,0x43300000); a.floating_sum_single(0,0,31); a.exclusive_flags(3,3,0x80000000);
                a.store(1,0xa8,3); a.floating_load(3,26,128); a.floating_product_single(0,0,31); a.store(1,0xa4,0);
                a.floating_load(4,31,356,true); a.floating_load(2,1,164); a.floating_product_single(1,31,0);
                a.floating_load(0,31,0,true); a.floating_subtract(2,2,3,true); a.load<1>(13,15,0x10c);
                a.floating_product_single(1,31,1); a.floating_product_single(1,2,1);
                a.floating_product_single(1,4,1); a.floating_compare(1,0);
                if (!a.equal()) {
                    a.at(0x1027cc00); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027cc18); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027cc28);
                    primary_phase_255(a,state);
                } else {
                    a.at(0x1027cbf8); a.value(14,0x0); a.jump(0x1027cfdc);
                    primary_phase_258(a,state);
                }
            }
        }
    }
}
void primary_phase_253(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cb6c); a.value(0,0x1); a.jump(0x1027cb78);
    primary_phase_254(a,state);
}
void primary_phase_252(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cb44); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027d43c);
        primary_phase_263(a,state);
    } else {
        a.at(0x1027cb4c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x1027cb6c);
            primary_phase_253(a,state);
        } else {
            a.at(0x1027cb5c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x1027cb74); a.value(0,0x0);
                primary_phase_254(a,state);
            } else {
                a.at(0x1027cb6c);
                primary_phase_253(a,state);
            }
        }
    }
}
void primary_phase_251(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027cb40); a.sign8(0,3);
    primary_phase_252(a,state);
}
void primary_phase_250(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027caf4); a.sign8(0,3); a.jump(0x1027cb44);
    primary_phase_252(a,state);
}
void primary_phase_249(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ca90); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027cafc); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027cb10); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027cb20); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027cb2c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027cb3c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027cb40);
        primary_phase_251(a,state);
    } else {
        a.at(0x1027caac); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027cac0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027cad0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027cae0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027caf0); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027caf4);
        primary_phase_250(a,state);
    }
}
void primary_phase_248(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ca8c); a.offset(19,19,-0xbb8);
    primary_phase_249(a,state);
}
void primary_phase_247(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ca84); a.offset(21,21,-0x28a); a.jump(0x1027ca90);
    primary_phase_249(a,state);
}
void primary_phase_246(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ca80); a.offset(19,19,-0xbb8);
    primary_phase_247(a,state);
}
void primary_phase_245(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ca08); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.mask(0,3,0x38,true,0);
    if (a.equal()) {
        a.at(0x1027ca80);
        primary_phase_246(a,state);
    } else {
        a.at(0x1027ca1c); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.sign16(0,3);
        a.mask(0,0,0x880,true);
        if (a.equal()) {
            a.at(0x1027ca80);
            primary_phase_246(a,state);
        } else {
            a.at(0x1027ca34); a.load<1>(3,15,0x10c); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
            a.floating_load(2,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(3,15,152,true);
            a.store(1,0xa8,0); a.floating_load(4,31,92,true); a.floating_load(1,1,164);
            a.floating_load(0,31,248,true); a.floating_subtract(1,1,2,true); a.floating_sum_single(1,3,1);
            a.floating_product_difference(0,4,1,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(19,19,0);
            a.jump(0x1027ca84);
            primary_phase_247(a,state);
        }
    }
}
void primary_phase_244(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c9b8); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ca08);
        primary_phase_245(a,state);
    } else {
        a.at(0x1027c9c0); a.load<1>(3,15,0x10c); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
        a.floating_load(1,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(2,15,152,true);
        a.store(1,0xa8,0); a.floating_load(3,31,92,true); a.floating_load(0,1,164);
        a.floating_subtract(0,0,1,true); a.floating_sum_single(0,2,0); a.floating_product_single(0,3,0);
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(19,19,0); a.jump(0x1027ca84);
        primary_phase_247(a,state);
    }
}
void primary_phase_243(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c9b4); a.value(0,0x0);
    primary_phase_244(a,state);
}
void primary_phase_242(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c990); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c9b4);
        primary_phase_243(a,state);
    } else {
        a.at(0x1027c99c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027c9b4);
            primary_phase_243(a,state);
        } else {
            a.at(0x1027c9ac); a.value(0,0x1); a.jump(0x1027c9b8);
            primary_phase_244(a,state);
        }
    }
}
void primary_phase_241(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c940); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ca8c);
        primary_phase_248(a,state);
    } else {
        a.at(0x1027c948); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.mask(0,3,0x38,true,0);
        if (a.equal()) {
            a.at(0x1027ca08);
            primary_phase_245(a,state);
        } else {
            a.at(0x1027c95c); a.move(3,15); a.call(0x1028abc0); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x1027ca08);
                primary_phase_245(a,state);
            } else {
                a.at(0x1027c96c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
                if (!a.equal()) {
                    a.at(0x1027c990);
                    primary_phase_242(a,state);
                } else {
                    a.at(0x1027c978); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
                    if (a.at_least()) {
                        a.at(0x1027c990);
                        primary_phase_242(a,state);
                    } else {
                        a.at(0x1027c988); a.value(0,0x1); a.jump(0x1027c9b8);
                        primary_phase_244(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_240(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c93c); a.value(0,0x0);
    primary_phase_241(a,state);
}
void primary_phase_239(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c934); a.value(0,0x1); a.jump(0x1027c940);
    primary_phase_241(a,state);
}
void primary_phase_238(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c918); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c93c);
        primary_phase_240(a,state);
    } else {
        a.at(0x1027c924); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027c93c);
            primary_phase_240(a,state);
        } else {
            a.at(0x1027c934);
            primary_phase_239(a,state);
        }
    }
}
void primary_phase_237(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c8f4); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ca8c);
        primary_phase_248(a,state);
    } else {
        a.at(0x1027c8fc); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c918);
            primary_phase_238(a,state);
        } else {
            a.at(0x1027c908); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027c934);
                primary_phase_239(a,state);
            } else {
                a.at(0x1027c918);
                primary_phase_238(a,state);
            }
        }
    }
}
void primary_phase_236(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c8e8); a.value(0,0x1); a.jump(0x1027c8f4);
    primary_phase_237(a,state);
}
void primary_phase_235(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c8a4); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027c8ac); a.load<1>(0,15,0x10c); a.offset(19,19,0x3e8); a.sign8(0,0); a.multiply(0,0,0x64);
        a.subtract_from(0,0,0x7d0); a.subtract(21,0,21); a.jump(0x1027ca90);
    }
    a.at(0x1027ca90);
    primary_phase_249(a,state);
}
void primary_phase_234(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c8a0); a.sign8(0,3);
    primary_phase_235(a,state);
}
void primary_phase_233(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c854); a.sign8(0,3); a.jump(0x1027c8a4);
    primary_phase_235(a,state);
}
void primary_phase_232(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c7e8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ca90);
        primary_phase_249(a,state);
    } else {
        a.at(0x1027c7f0); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c85c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027c870); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027c880); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x1027c88c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027c89c); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027c8a0);
            primary_phase_234(a,state);
        } else {
            a.at(0x1027c80c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027c820); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027c830); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x1027c840); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027c850); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027c854);
            primary_phase_233(a,state);
        }
    }
}
void primary_phase_231(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c7e4); a.value(0,0x0);
    primary_phase_232(a,state);
}
void primary_phase_230(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c7dc); a.value(0,0x1); a.jump(0x1027c7e8);
    primary_phase_232(a,state);
}
void primary_phase_229(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c7c0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c7e4);
        primary_phase_231(a,state);
    } else {
        a.at(0x1027c7cc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027c7e4);
            primary_phase_231(a,state);
        } else {
            a.at(0x1027c7dc);
            primary_phase_230(a,state);
        }
    }
}
void primary_phase_228(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c7a4); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c7c0);
        primary_phase_229(a,state);
    } else {
        a.at(0x1027c7b0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
        if (a.less()) {
            a.at(0x1027c7dc);
            primary_phase_230(a,state);
        } else {
            a.at(0x1027c7c0);
            primary_phase_229(a,state);
        }
    }
}
void primary_phase_227(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c790); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c7a4);
        primary_phase_228(a,state);
    } else {
        a.at(0x1027c798); a.offset(19,19,0x4e2); a.offset(21,21,-0x1f4); a.jump(0x1027ca90);
        primary_phase_249(a,state);
    }
}
void primary_phase_226(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c78c); a.value(0,0x0);
    primary_phase_227(a,state);
}
void primary_phase_225(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c784); a.value(0,0x1); a.jump(0x1027c790);
    primary_phase_227(a,state);
}
void primary_phase_224(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c768); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c78c);
        primary_phase_226(a,state);
    } else {
        a.at(0x1027c774); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027c78c);
            primary_phase_226(a,state);
        } else {
            a.at(0x1027c784);
            primary_phase_225(a,state);
        }
    }
}
void primary_phase_223(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c744); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c7a4);
        primary_phase_228(a,state);
    } else {
        a.at(0x1027c74c); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c768);
            primary_phase_224(a,state);
        } else {
            a.at(0x1027c758); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x1027c784);
                primary_phase_225(a,state);
            } else {
                a.at(0x1027c768);
                primary_phase_224(a,state);
            }
        }
    }
}
void primary_phase_222(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c740); a.value(0,0x0);
    primary_phase_223(a,state);
}
void primary_phase_221(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c738); a.value(0,0x1); a.jump(0x1027c744);
    primary_phase_223(a,state);
}
void primary_phase_220(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c71c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c740);
        primary_phase_222(a,state);
    } else {
        a.at(0x1027c728); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027c740);
            primary_phase_222(a,state);
        } else {
            a.at(0x1027c738);
            primary_phase_221(a,state);
        }
    }
}
void primary_phase_219(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c6f8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027c7a4);
        primary_phase_228(a,state);
    } else {
        a.at(0x1027c700); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c71c);
            primary_phase_220(a,state);
        } else {
            a.at(0x1027c70c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027c738);
                primary_phase_221(a,state);
            } else {
                a.at(0x1027c71c);
                primary_phase_220(a,state);
            }
        }
    }
}
void primary_phase_218(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c6ec); a.value(0,0x1); a.jump(0x1027c6f8);
    primary_phase_219(a,state);
}
void primary_phase_217(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c6c8); a.load<1>(0,15,0x105); a.offset(20,20,-0xbb8); a.sign8(0,0); a.compare(0,2);
    if (a.less()) {
        a.at(0x1027c6ec);
        primary_phase_218(a,state);
    } else {
        a.at(0x1027c6dc); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
        if (a.nonpositive()) {
            a.at(0x1027c6f4); a.value(0,0x0);
            primary_phase_219(a,state);
        } else {
            a.at(0x1027c6ec);
            primary_phase_218(a,state);
        }
    }
}
void primary_phase_216(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c6c4); a.store<1>(1,0x4d,0);
    primary_phase_217(a,state);
}
void primary_phase_215(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c634); a.load<1>(0,1,0xff); a.offset(6,1,0x4c); a.load<1>(4,1,0x53); a.sign8(3,0);
    a.load<1>(5,1,0x52); a.negate(0,3); a.store<1>(1,0x4d,4); a.combine(0,0,3); a.move(4,15); a.store<1>(1,0x4c,5);
    a.mask(3,0,0x1,false,1); a.offset(0,3,0x2); a.offset(5,1,0x4d); a.load(3,15,0x1a4); a.sign8(7,0);
    a.call(0x101d8260); a.move(18,3); a.sign8(0,18,true);
    if (a.equal()) {
        a.at(0x1027c8c8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x1027c8e8);
            primary_phase_236(a,state);
        } else {
            a.at(0x1027c8d8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x1027c8f0); a.value(0,0x0);
                primary_phase_237(a,state);
            } else {
                a.at(0x1027c8e8);
                primary_phase_236(a,state);
            }
        }
    } else {
        a.at(0x1027c680); a.sign8(0,16); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x1027c6c8);
            primary_phase_217(a,state);
        } else {
            a.at(0x1027c68c); a.load<1>(0,1,0x4d); a.sign8(0,0); a.compare(0,3);
            if (a.at_least()) {
                a.at(0x1027c6a4); a.load<1>(0,1,0x4d); a.sign8(0,0); a.compare(0,5);
                if (a.nonpositive()) {
                    a.at(0x1027c6bc); a.load<1>(0,1,0x4d); a.sign8(0,0);
                } else {
                    a.at(0x1027c6b4); a.value(0,0x5); a.jump(0x1027c6c4);
                }
            } else {
                a.at(0x1027c69c); a.value(0,0x3); a.jump(0x1027c6c4);
            }
            a.at(0x1027c6c4);
            primary_phase_216(a,state);
        }
    }
}
void primary_phase_214(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c620); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027c628); a.load(0,1,0x68); a.subtract_from(0,0,0x7d0); a.add(19,19,0);
    }
    a.at(0x1027c634);
    primary_phase_215(a,state);
}
void primary_phase_213(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c61c); a.value(0,0x0);
    primary_phase_214(a,state);
}
void primary_phase_212(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c614); a.value(0,0x1); a.jump(0x1027c620);
    primary_phase_214(a,state);
}
void primary_phase_211(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c5f8); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c61c);
        primary_phase_213(a,state);
    } else {
        a.at(0x1027c604); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027c61c);
            primary_phase_213(a,state);
        } else {
            a.at(0x1027c614);
            primary_phase_212(a,state);
        }
    }
}
void primary_phase_210(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c5d4); a.sign8(0,3,true);
    if (!a.equal()) {
        a.at(0x1027c634);
        primary_phase_215(a,state);
    } else {
        a.at(0x1027c5dc); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c5f8);
            primary_phase_211(a,state);
        } else {
            a.at(0x1027c5e8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x1027c614);
                primary_phase_212(a,state);
            } else {
                a.at(0x1027c5f8);
                primary_phase_211(a,state);
            }
        }
    }
}
void primary_phase_209(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c5b4); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0x0); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60);
    primary_phase_210(a,state);
}
void primary_phase_208(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c55c); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0xb); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60); a.jump(0x1027c5d4);
    primary_phase_210(a,state);
}
void primary_phase_207(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c51c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c580); a.load<1>(0,15,0x106); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027c5b4);
            primary_phase_209(a,state);
        } else {
            a.at(0x1027c58c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027c5b4);
                primary_phase_209(a,state);
            } else {
                a.at(0x1027c59c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                if (a.positive()) {
                    a.at(0x1027c5b4);
                    primary_phase_209(a,state);
                } else {
                    a.at(0x1027c5ac); a.value(3,0x1); a.jump(0x1027c5d4);
                    primary_phase_210(a,state);
                }
            }
        }
    } else {
        a.at(0x1027c528); a.load<1>(0,15,0x106); a.compare(0,11);
        if (!a.equal()) {
            a.at(0x1027c55c);
            primary_phase_208(a,state);
        } else {
            a.at(0x1027c534); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027c55c);
                primary_phase_208(a,state);
            } else {
                a.at(0x1027c544); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                if (a.positive()) {
                    a.at(0x1027c55c);
                    primary_phase_208(a,state);
                } else {
                    a.at(0x1027c554); a.value(3,0x1); a.jump(0x1027c5d4);
                    primary_phase_210(a,state);
                }
            }
        }
    }
}
void primary_phase_206(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c500); a.floating_load(1,31,352,true); a.floating_load(0,31,348,true);
    a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.subtract(20,0,20);
    primary_phase_207(a,state);
}
void primary_phase_205(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c4c8); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c500);
        primary_phase_206(a,state);
    } else {
        a.at(0x1027c4d0); a.floating_load(1,15,152,true); a.floating_load(0,31,240,true);
        a.floating_load(2,26,552); a.floating_subtract(0,1,0,true); a.floating_load(1,26,912);
        a.floating_product_sum(0,2,29,0); a.floating_product(0,1,0); a.floating_integer(0,0);
        a.floating_store(0,1,164); a.load(0,1,0xa8); a.add(19,19,0); a.jump(0x1027c51c);
        primary_phase_207(a,state);
    }
}
void primary_phase_204(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c4c4); a.sign8(0,3);
    primary_phase_205(a,state);
}
void primary_phase_203(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c478); a.sign8(0,3); a.jump(0x1027c4c8);
    primary_phase_205(a,state);
}
void primary_phase_202(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c40c); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.sign16(0,3); a.compare(0,1);
    if (a.equal()) {
        a.at(0x1027c500);
        primary_phase_206(a,state);
    } else {
        a.at(0x1027c424); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c480); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027c494); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027c4a4); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x1027c4b0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027c4c0); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027c4c4);
            primary_phase_204(a,state);
        } else {
            a.at(0x1027c430); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027c444); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027c454); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x1027c464); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027c474); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027c478);
            primary_phase_203(a,state);
        }
    }
}
void primary_phase_201(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c3f4); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027c3fc); a.load<2>(0,1,0x58,true); a.multiply(3,0,0x64); a.offset(0,3,0x1f4); a.subtract(19,0,19);
    }
    a.at(0x1027c40c);
    primary_phase_202(a,state);
}
void primary_phase_200(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c3d4); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0x0); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60);
    primary_phase_201(a,state);
}
void primary_phase_199(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c37c); a.floating_load(1,31,0,true); a.move(3,15); a.floating_load(2,31,0,true); a.value(4,0x4);
    a.value(5,0xb); a.value(6,0x0); a.value(9,0x0); a.call(0x10244c60); a.jump(0x1027c3f4);
    primary_phase_201(a,state);
}
void primary_phase_198(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c334); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027c40c);
        primary_phase_202(a,state);
    } else {
        a.at(0x1027c33c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027c3a0); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027c3d4);
                primary_phase_200(a,state);
            } else {
                a.at(0x1027c3ac); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x1027c3d4);
                    primary_phase_200(a,state);
                } else {
                    a.at(0x1027c3bc); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x1027c3d4);
                        primary_phase_200(a,state);
                    } else {
                        a.at(0x1027c3cc); a.value(3,0x1); a.jump(0x1027c3f4);
                        primary_phase_201(a,state);
                    }
                }
            }
        } else {
            a.at(0x1027c348); a.load<1>(0,15,0x106); a.compare(0,11);
            if (!a.equal()) {
                a.at(0x1027c37c);
                primary_phase_199(a,state);
            } else {
                a.at(0x1027c354); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
                if (a.less()) {
                    a.at(0x1027c37c);
                    primary_phase_199(a,state);
                } else {
                    a.at(0x1027c364); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,5);
                    if (a.positive()) {
                        a.at(0x1027c37c);
                        primary_phase_199(a,state);
                    } else {
                        a.at(0x1027c374); a.value(3,0x1); a.jump(0x1027c3f4);
                        primary_phase_201(a,state);
                    }
                }
            }
        }
    }
}
void primary_phase_197(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c330); a.value(0,0x0);
    primary_phase_198(a,state);
}
void primary_phase_196(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c328); a.value(0,0x1); a.jump(0x1027c334);
    primary_phase_198(a,state);
}
void primary_phase_195(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c30c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c330);
        primary_phase_197(a,state);
    } else {
        a.at(0x1027c318); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027c330);
            primary_phase_197(a,state);
        } else {
            a.at(0x1027c328);
            primary_phase_196(a,state);
        }
    }
}
void primary_phase_194(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c2dc); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027c2f0); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027c30c);
            primary_phase_195(a,state);
        } else {
            a.at(0x1027c2fc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x1027c328);
                primary_phase_196(a,state);
            } else {
                a.at(0x1027c30c);
                primary_phase_195(a,state);
            }
        }
    } else {
        a.at(0x1027c2e4); a.offset(20,20,-0x3e8); a.offset(19,19,-0x4b0); a.jump(0x1027c40c);
        primary_phase_202(a,state);
    }
}
void primary_phase_193(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c2d8); a.sign8(0,3);
    primary_phase_194(a,state);
}
void primary_phase_192(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c28c); a.sign8(0,3); a.jump(0x1027c2dc);
    primary_phase_194(a,state);
}
void primary_phase_191(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c238); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c294); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c2a8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c2b8); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027c2c4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c2d4); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c2d8);
        primary_phase_193(a,state);
    } else {
        a.at(0x1027c244); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c258); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c268); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027c278); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c288); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c28c);
        primary_phase_192(a,state);
    }
}
void primary_phase_190(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c22c); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c234); a.offset(20,20,-0x3e8);
    }
    a.at(0x1027c238);
    primary_phase_191(a,state);
}
void primary_phase_189(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c228); a.sign8(0,3);
    primary_phase_190(a,state);
}
void primary_phase_188(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c1dc); a.sign8(0,3); a.jump(0x1027c22c);
    primary_phase_190(a,state);
}
void primary_phase_187(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c178); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c1e4); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c1f8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c208); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027c214); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c224); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c228);
        primary_phase_189(a,state);
    } else {
        a.at(0x1027c194); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c1a8); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c1b8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027c1c8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c1d8); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c1dc);
        primary_phase_188(a,state);
    }
}
void primary_phase_186(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c164); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c16c); a.offset(20,20,-0x3e8); a.offset(21,21,0x3e8); a.jump(0x1027c40c);
    }
    a.at(0x1027c40c);
    primary_phase_202(a,state);
}
void primary_phase_185(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c160); a.sign8(0,3);
    primary_phase_186(a,state);
}
void primary_phase_184(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c114); a.sign8(0,3); a.jump(0x1027c164);
    primary_phase_186(a,state);
}
void primary_phase_183(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c0b0); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c11c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c130); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c140); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027c14c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c15c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c160);
        primary_phase_185(a,state);
    } else {
        a.at(0x1027c0cc); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027c0e0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027c0f0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027c100); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027c110); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027c114);
        primary_phase_184(a,state);
    }
}
void primary_phase_182(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c0a4); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c0ac); a.offset(21,21,-0xbb8);
    }
    a.at(0x1027c0b0);
    primary_phase_183(a,state);
}
void primary_phase_181(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c0a0); a.value(0,0x0);
    primary_phase_182(a,state);
}
void primary_phase_180(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c098); a.value(0,0x1); a.jump(0x1027c0a4);
    primary_phase_182(a,state);
}
void primary_phase_179(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c07c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027c0a0);
        primary_phase_181(a,state);
    } else {
        a.at(0x1027c088); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027c0a0);
            primary_phase_181(a,state);
        } else {
            a.at(0x1027c098);
            primary_phase_180(a,state);
        }
    }
}
void primary_phase_178(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c060); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027c07c);
        primary_phase_179(a,state);
    } else {
        a.at(0x1027c06c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027c098);
            primary_phase_180(a,state);
        } else {
            a.at(0x1027c07c);
            primary_phase_179(a,state);
        }
    }
}
void primary_phase_177(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027c018); a.floating_load(1,31,220,true); a.sign8(0,16); a.floating_load(0,31,220,true);
    a.compare(0,4); a.offset(20,20,-0x1388);
    a.floating_product_difference(0,1,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.add(19,19,0);
    if (a.equal()) {
        a.at(0x1027c044); a.floating_load(1,31,216,true); a.floating_load(0,31,220,true);
        a.floating_product_difference(0,1,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(19,19,0);
    }
    a.at(0x1027c060);
    primary_phase_178(a,state);
}
void primary_phase_176(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bfc4); a.load(3,15,0x1a4); a.load<1>(4,15,0x27); a.load<1>(5,15,0x106); a.call(0x101e4ea0);
    a.sign8(0,3,true);
    if (a.at_least()) {
        a.at(0x1027c018);
        primary_phase_177(a,state);
    } else {
        a.at(0x1027bfdc); a.load(3,15,0x1a4); a.move(4,15); a.value(5,0x0); a.call(0x101d8d70); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x1027c178);
            primary_phase_187(a,state);
        } else {
            a.at(0x1027bff4); a.load<1>(0,1,0x100); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x1027c178);
                primary_phase_187(a,state);
            } else {
                a.at(0x1027c000); a.load(3,15,0x1a4); a.move(4,15); a.call(0x101e3d50); a.sign16(0,3);
                a.compare(0,1);
                if (a.equal()) {
                    a.at(0x1027c178);
                    primary_phase_187(a,state);
                } else {
                    a.at(0x1027c018);
                    primary_phase_177(a,state);
                }
            }
        }
    }
}
void primary_phase_175(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bf6c); a.compare_registers(13,14);
    if (a.nonpositive()) {
        a.at(0x1027bfc4);
        primary_phase_176(a,state);
    } else {
        a.at(0x1027bf74); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x1027bfc4);
            primary_phase_176(a,state);
        } else {
            a.at(0x1027bf88); a.sign8(0,16); a.compare(0,4);
            if (a.equal()) {
                a.at(0x1027bfc4);
                primary_phase_176(a,state);
            } else {
                a.at(0x1027bf94); a.move(3,15); a.call(0x10285790); a.move(4,3); a.move(3,15); a.value(5,0x0);
                a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20);
                a.move(3,13); a.jump(0x1028175c);
            }
        }
    }
}
void primary_phase_174(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bf68); a.move(14,0);
    primary_phase_175(a,state);
}
void primary_phase_173(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027be58); a.exclusive_flags(3,18,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,18,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,18,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027bf34); a.offset(14,18,-0x1); a.jump(0x1027bf6c);
    }
    a.at(0x1027bf6c);
    primary_phase_175(a,state);
}
void primary_phase_172(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027be38); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(18,3);
    if (a.positive()) {
        a.at(0x1027be58);
        primary_phase_173(a,state);
    } else {
        a.at(0x1027be48); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(18,3);
        if (a.at_least()) {
            a.at(0x1027bf3c); a.load(3,30,0x0); a.compare(18,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,18); a.product(0,0,18); a.subtract(14,0,3); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027bf64); a.move(0,14);
            }
            a.at(0x1027bf68);
            primary_phase_174(a,state);
        } else {
            a.at(0x1027be58);
            primary_phase_173(a,state);
        }
    }
}
void primary_phase_171(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bdfc); a.load(3,1,0x68); a.offset_record(18,3,0x7d0);
    if (!a.equal()) {
        a.at(0x1027be10); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027be28); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027be38);
        primary_phase_172(a,state);
    } else {
        a.at(0x1027be08); a.value(14,0x0); a.jump(0x1027bf6c);
        primary_phase_175(a,state);
    }
}
void primary_phase_170(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bdf8); a.move(13,0);
    primary_phase_171(a,state);
}
void primary_phase_169(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bc58); a.floating_load(1,31,176,true); a.value(4,0x43300000); a.floating_load(0,31,332,true);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(0,0x43300000);
    a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.store(1,0xac,4); a.value(5,0x10000); a.value(3,0x80010000); a.load<2>(7,7,0x0); a.offset(6,6,-0x7fff);
    a.floating_integer(0,0); a.load(4,27,0x0); a.floating_load(4,26,128); a.offset(5,5,-0x1); a.add(7,7,4);
    a.floating_load(2,26,0); a.floating_store(0,1,164); a.multiply_high(6,6,7); a.floating_load(1,31,176,true);
    a.offset(3,3,-0x7fff); a.load(4,1,0xa8); a.store(1,0xb4,0); a.exclusive_flags(4,4,0x80000000);
    a.store(1,0xb0,4); a.add(0,6,7); a.shift_right(0,0,0xf); a.floating_load(0,31,332,true);
    a.floating_load(3,1,172); a.mask(4,0,0x1,false,1);
    a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.add(0,0,4); a.floating_subtract(3,3,4); a.product(0,0,5); a.floating_load(1,26,128); a.floating_integer(0,0);
    a.floating_difference(2,3,2,true); a.subtract(13,0,7); a.floating_store(0,1,196);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb8,4); a.load(0,1,0xc8); a.floating_load(0,1,180);
    a.multiply(0,0,0xa); a.floating_subtract(0,0,1); a.multiply_high(3,3,0); a.floating_product(0,2,0);
    a.add(0,3,0); a.floating_integer(0,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.floating_store(0,1,188); a.load(13,1,0xc0); a.call(0x10226520); a.floating_load(1,31,176,true);
    a.value(4,0x66660000); a.floating_load(0,31,332,true); a.offset(0,3,0x5); a.offset(4,4,0x6667);
    a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.multiply_high(0,4,0); a.floating_integer(0,0); a.shift_right(0,0,0x2); a.floating_store(0,1,204);
    a.mask(4,0,0x1,false,1); a.add(0,0,4); a.load(3,1,0xd0); a.add(13,13,0); a.offset(0,3,-0x1);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027bd7c); a.floating_load(1,31,176,true); a.floating_load(0,31,332,true);
        a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8); a.offset(13,3,-0x1);
        a.jump(0x1027bdfc);
    }
    a.at(0x1027bdfc);
    primary_phase_171(a,state);
}
void primary_phase_168(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bc08); a.floating_load(1,31,176,true); a.value(3,0x10000); a.floating_load(0,31,332,true);
    a.offset(3,3,-0x1);
    a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027bc58);
        primary_phase_169(a,state);
    } else {
        a.at(0x1027bc30); a.floating_load(1,31,176,true); a.value(3,0xffff0000); a.floating_load(0,31,332,true);
        a.offset(3,3,0x1);
        a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027bd9c); a.floating_load(1,31,176,true); a.floating_load(0,31,332,true); a.load(3,30,0x0);
            a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.load(0,27,0x0); a.load<2>(3,3,0x0); a.floating_load(1,31,176,true); a.floating_integer(2,0);
            a.floating_load(0,31,332,true); a.add(5,3,0);
            a.floating_product_difference(0,1,29,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.floating_store(2,1,164); a.load(4,1,0xa8); a.floating_integer(0,0); a.divide(3,5,4);
            a.floating_store(0,1,172); a.load(0,1,0xb0); a.compare(0,0); a.product(0,3,4); a.subtract(13,0,5);
            a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027bdf4); a.move(0,13);
            }
            a.at(0x1027bdf8);
            primary_phase_170(a,state);
        } else {
            a.at(0x1027bc58);
            primary_phase_169(a,state);
        }
    }
}
void primary_phase_167(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bbc0); a.floating_load(2,31,176,true); a.floating_load(1,31,332,true); a.floating_load(0,31,0,true);
    a.floating_product_difference(1,2,29,1); { const auto rounded=-double(float(float_value<8>(state.r<8>(16400)))); state.wr<8>(16400,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x1027bbe0); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027bbf8); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027bc08);
        primary_phase_168(a,state);
    } else {
        a.at(0x1027bbd8); a.value(13,0x0); a.jump(0x1027bdfc);
        primary_phase_171(a,state);
    }
}
void primary_phase_166(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bb8c); a.load<1>(0,15,0x27); a.load(3,15,0x1a4); a.sign8(0,0); a.load<1>(5,15,0x106);
    a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.call(0x101ee9f0); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027bfc4);
        primary_phase_176(a,state);
    } else {
        a.at(0x1027bbb4); a.floating_load(0,26,16); a.floating_compare(29,0);
        if (a.at_least()) {
            a.at(0x1027bfc4);
            primary_phase_176(a,state);
        } else {
            a.at(0x1027bbc0);
            primary_phase_167(a,state);
        }
    }
}
void primary_phase_165(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bb74); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027bb8c);
        primary_phase_166(a,state);
    } else {
        a.at(0x1027bb7c); a.floating_load(0,26,144); a.floating_compare(29,0); a.compare_less_or_equal_bit();
        if (a.equal()) {
            a.at(0x1027bbc0);
            primary_phase_167(a,state);
        } else {
            a.at(0x1027bb8c);
            primary_phase_166(a,state);
        }
    }
}
void primary_phase_164(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bb70); a.sign8(0,3);
    primary_phase_165(a,state);
}
void primary_phase_163(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bb24); a.sign8(0,3); a.jump(0x1027bb74);
    primary_phase_165(a,state);
}
void primary_phase_162(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027bad0); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027bb2c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027bb40); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027bb50); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027bb5c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027bb6c); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027bb70);
        primary_phase_164(a,state);
    } else {
        a.at(0x1027badc); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027baf0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027bb00); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027bb10); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027bb20); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027bb24);
        primary_phase_163(a,state);
    }
}
void primary_phase_161(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ba2c); a.load<1>(0,15,0x27); a.load(3,15,0x1a4); a.sign8(0,0); a.load<1>(5,15,0x106);
    a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.call(0x101ee9f0); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1027bad0);
        primary_phase_162(a,state);
    } else {
        a.at(0x1027ba54); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x1027bad0);
            primary_phase_162(a,state);
        } else {
            a.at(0x1027ba68); a.load<1>(0,15,0x109); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x1027bad0);
                primary_phase_162(a,state);
            } else {
                a.at(0x1027ba74); a.load(4,15,0x1a4); a.load(3,1,0x68); a.load<2>(0,4,0x7948,true);
                a.subtract_from(0,0,0xfa0); a.compare_registers(3,0);
                if (a.at_least()) {
                    a.at(0x1027bad0);
                    primary_phase_162(a,state);
                } else {
                    a.at(0x1027ba8c); a.load(3,15,0x1a4); a.load<1>(4,15,0x27); a.call(0x101fd000);
                    a.sign8(0,3,true);
                    if (a.equal()) {
                        a.at(0x1027bad0);
                        primary_phase_162(a,state);
                    } else {
                        a.at(0x1027baa0); a.move(3,15); a.call(0x10285790); a.move(4,3); a.move(3,15);
                        a.value(5,0x1); a.call(0x10245870); a.offset(3,1,0x74); a.value(13,0x0);
                        a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
                    }
                }
            }
        }
    }
}
void primary_phase_160(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ba0c); a.compare(22,0,false);
    if (a.equal()) {
        a.at(0x1027ba14); a.load<1>(0,1,0x100); a.sign8(0,0,true);
        if (!a.equal()) {
        } else {
            a.at(0x1027ba20); a.load(3,1,0x68); a.offset(0,3,-0x7d0); a.store(1,0x68,0);
        }
    }
    a.at(0x1027ba2c);
    primary_phase_161(a,state);
}
void primary_phase_159(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b9f0); a.load<1>(0,15,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540);
    a.multiply(0,3,0x7d0); a.subtract_from(0,0,0x1388); a.add(21,21,0);
    primary_phase_160(a,state);
}
void primary_phase_158(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b9d8); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ba0c);
        primary_phase_160(a,state);
    } else {
        a.at(0x1027b9e4); a.load<1>(0,15,0x106); a.compare(0,11);
        if (!a.equal()) {
            a.at(0x1027ba0c);
            primary_phase_160(a,state);
        } else {
            a.at(0x1027b9f0);
            primary_phase_159(a,state);
        }
    }
}
void primary_phase_157(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b9a0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,3);
    if (a.nonpositive()) {
        a.at(0x1027ba0c);
        primary_phase_160(a,state);
    } else {
        a.at(0x1027b9b0); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,7);
        if (a.at_least()) {
            a.at(0x1027ba0c);
            primary_phase_160(a,state);
        } else {
            a.at(0x1027b9c0); a.load<1>(0,15,0x27); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x1027b9d8);
                primary_phase_158(a,state);
            } else {
                a.at(0x1027b9cc); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x1027b9f0);
                    primary_phase_159(a,state);
                } else {
                    a.at(0x1027b9d8);
                    primary_phase_158(a,state);
                }
            }
        }
    }
}
void primary_phase_156(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b964); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
    if (!a.equal()) {
        a.at(0x1027b9a0);
        primary_phase_157(a,state);
    } else {
        a.at(0x1027b974); a.floating_load(0,26,16); a.floating_compare(31,0);
        if (a.nonpositive()) {
            a.at(0x1027b9a0);
            primary_phase_157(a,state);
        } else {
            a.at(0x1027b980); a.floating_product_single(0,31,31); a.floating_load(1,26,904);
            a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
            a.subtract(21,0,21); a.jump(0x1027ba0c);
            primary_phase_160(a,state);
        }
    }
}
void primary_phase_155(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b938); a.sign8(0,13); a.compare_registers(0,14);
    if (a.positive()) {
        a.at(0x1027b944); a.floating_load(2,31,128,true); a.floating_load(1,15,152,true);
        a.floating_load(0,31,176,true);
        a.floating_product_difference(0,2,1,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.add(21,21,0);
    }
    a.at(0x1027b964);
    primary_phase_156(a,state);
}
void primary_phase_154(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b934); a.move(14,0);
    primary_phase_155(a,state);
}
void primary_phase_153(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b7ac); a.floating_load(0,31,92,true); a.value(0,0x43300000); a.load(7,30,0x0);
    a.value(6,0x80010000); a.floating_product_single(1,0,31); a.floating_load(0,31,92,true); a.load<2>(8,7,0x0);
    a.value(4,0x43300000); a.floating_product_single(0,0,31); a.load(7,27,0x0); a.floating_integer(1,1);
    a.value(5,0x10000); a.floating_integer(0,0); a.value(3,0x80010000); a.store(1,0xac,0); a.add(7,8,7);
    a.floating_store(1,1,164); a.offset(6,6,-0x7fff); a.floating_load(3,26,128); a.multiply_high(6,6,7);
    a.load(0,1,0xa8); a.offset(5,5,-0x1); a.store(1,0xb4,4); a.offset(3,3,-0x7fff);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0xb0,0); a.add(4,6,7); a.floating_load(2,26,0);
    a.shift_right(4,4,0xf); a.floating_store(0,1,196); a.mask(6,4,0x1,false,1); a.floating_load(0,1,172);
    a.add(4,4,6); a.product(4,4,5); a.load(0,1,0xc8); a.floating_subtract(0,0,3); a.floating_load(1,26,128);
    a.floating_difference(2,0,2,true); a.subtract(14,4,7); a.exclusive_flags(4,14,0x80000000); a.store(1,0xb8,4);
    a.multiply(0,0,0xa); a.floating_load(0,1,180); a.floating_subtract(0,0,1); a.multiply_high(3,3,0);
    a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_integer(0,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_store(0,1,188); a.load(14,1,0xc0); a.call(0x10226520);
    a.floating_load(0,31,92,true); a.value(4,0x66660000); a.offset(0,3,0x5); a.floating_product_single(0,0,31);
    a.offset(4,4,0x6667); a.multiply_high(0,4,0); a.floating_integer(0,0); a.floating_store(0,1,204);
    a.shift_right(0,0,0x2); a.mask(4,0,0x1,false,1); a.load(3,1,0xd0); a.add(0,0,4); a.add(14,14,0);
    a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027b8c4); a.floating_load(0,31,92,true); a.floating_product_single(0,0,31);
        a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8); a.offset(14,3,-0x1);
        a.jump(0x1027b938);
    }
    a.at(0x1027b938);
    primary_phase_155(a,state);
}
void primary_phase_152(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b764); a.floating_load(0,31,92,true); a.value(3,0x10000); a.offset(3,3,-0x1);
    a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027b7ac);
        primary_phase_153(a,state);
    } else {
        a.at(0x1027b788); a.floating_load(0,31,92,true); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027b8e0); a.floating_load(0,31,92,true); a.load(3,30,0x0); a.floating_product_single(1,0,31);
            a.floating_load(0,31,92,true); a.load<2>(3,3,0x0); a.load(0,27,0x0); a.floating_product_single(0,0,31);
            a.floating_integer(1,1); a.add(5,3,0); a.floating_integer(0,0); a.floating_store(1,1,164);
            a.load(4,1,0xa8); a.floating_store(0,1,172); a.divide(3,5,4); a.load(0,1,0xb0); a.compare(0,0);
            a.product(0,3,4); a.subtract(14,0,5); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027b930); a.move(0,14);
            }
            a.at(0x1027b934);
            primary_phase_154(a,state);
        } else {
            a.at(0x1027b7ac);
            primary_phase_153(a,state);
        }
    }
}
void primary_phase_151(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b688); a.load<1>(0,23,0x105); a.load<1>(3,15,0x105); a.sign8(0,0); a.sign8(3,3);
    a.compare_registers(3,0);
    if (a.equal()) {
        a.at(0x1027b6a0); a.load<1>(0,23,0x106); a.load<1>(3,15,0x106); a.sign8(0,0); a.sign8(3,3);
        a.compare_registers(3,0);
        if (!a.equal()) {
        } else {
            a.at(0x1027b6b8); a.load<1>(3,15,0x17); a.value(0,0x43300000); a.store(1,0xa4,0); a.sign8(0,3);
            a.floating_load(2,26,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(3,15,152,true);
            a.store(1,0xa8,0); a.floating_load(4,31,28,true); a.floating_load(1,1,164);
            a.floating_load(0,31,332,true); a.floating_subtract(1,1,2,true); a.floating_sum_single(1,3,1);
            a.floating_product_difference(0,4,1,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.add(19,19,0);
            a.jump(0x1027b964);
        }
    }
    a.at(0x1027b964);
    primary_phase_156(a,state);
}
void primary_phase_150(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b64c); a.load<1>(4,15,0x17); a.sign8(3,13); a.offset(0,3,-0xa); a.sign8(3,4); a.add(0,3,0);
    a.compare_registers(0,14);
    if (a.positive()) {
        a.at(0x1027b668); a.floating_load(2,31,128,true); a.floating_load(1,15,152,true);
        a.floating_load(0,31,220,true);
        a.floating_product_difference(0,2,1,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.add(21,21,0);
    }
    a.at(0x1027b688);
    primary_phase_151(a,state);
}
void primary_phase_149(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b648); a.move(14,0);
    primary_phase_150(a,state);
}
void primary_phase_148(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b4a8); a.floating_load(0,31,228,true); a.value(0,0x43300000); a.value(4,0x43300000);
    a.load(7,30,0x0); a.floating_product_single(1,0,31); a.floating_load(0,31,228,true); a.store(1,0xac,4);
    a.value(6,0x80010000); a.floating_product_single(0,0,31); a.value(5,0x10000);
    a.floating_product_single(1,1,31); a.value(3,0x80010000); a.floating_product_single(0,0,31); a.store(1,0xb4,0);
    a.floating_load(3,26,128); a.offset(6,6,-0x7fff); a.floating_integer(1,1); a.floating_load(2,26,0);
    a.floating_integer(0,0); a.load<2>(7,7,0x0); a.load(4,27,0x0); a.offset(5,5,-0x1); a.floating_store(1,1,164);
    a.add(7,7,4); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.load(0,1,0xa8); a.offset(3,3,-0x7fff);
    a.floating_store(0,1,196); a.exclusive_flags(0,0,0x80000000); a.store(1,0xb0,0); a.add(0,4,7);
    a.shift_right(4,0,0xf); a.load(0,1,0xc8); a.floating_load(0,1,172); a.mask(6,4,0x1,false,1); a.add(4,4,6);
    a.floating_subtract(0,0,3); a.product(4,4,5); a.floating_difference(2,0,2,true); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xb8,4); a.multiply(0,0,0xa); a.floating_load(0,1,180);
    a.floating_subtract(0,0,1); a.multiply_high(3,3,0); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_integer(0,0); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.floating_store(0,1,188); a.load(14,1,0xc0); a.call(0x10226520); a.floating_load(0,31,228,true);
    a.value(4,0x66660000); a.offset(0,3,0x5); a.floating_product_single(0,0,31); a.offset(4,4,0x6667);
    a.multiply_high(0,4,0); a.floating_product_single(0,0,31); a.floating_integer(0,0); a.shift_right(0,0,0x2);
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.floating_store(0,1,204); a.add(14,14,0); a.load(3,1,0xd0);
    a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027b5cc); a.floating_load(0,31,228,true); a.floating_product_single(0,0,31);
        a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8);
        a.offset(14,3,-0x1); a.jump(0x1027b64c);
    }
    a.at(0x1027b64c);
    primary_phase_150(a,state);
}
void primary_phase_147(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b458); a.floating_load(0,31,228,true); a.value(3,0x10000); a.offset(3,3,-0x1);
    a.floating_product_single(0,0,31); a.floating_product_single(0,0,31); a.floating_integer(0,0);
    a.floating_store(0,1,164); a.load(0,1,0xa8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027b4a8);
        primary_phase_148(a,state);
    } else {
        a.at(0x1027b480); a.floating_load(0,31,228,true); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_product_single(0,0,31); a.floating_product_single(0,0,31); a.floating_integer(0,0);
        a.floating_store(0,1,164); a.load(0,1,0xa8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027b5ec); a.floating_load(0,31,228,true); a.load(3,30,0x0); a.floating_product_single(1,0,31);
            a.floating_load(0,31,228,true); a.load<2>(3,3,0x0); a.load(0,27,0x0);
            a.floating_product_single(0,0,31); a.floating_product_single(1,1,31); a.add(5,3,0);
            a.floating_product_single(0,0,31); a.floating_integer(1,1); a.floating_integer(0,0);
            a.floating_store(1,1,164); a.load(4,1,0xa8); a.floating_store(0,1,172); a.divide(3,5,4);
            a.load(0,1,0xb0); a.compare(0,0); a.product(0,3,4); a.subtract(14,0,5); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027b644); a.move(0,14);
            }
            a.at(0x1027b648);
            primary_phase_149(a,state);
        } else {
            a.at(0x1027b4a8);
            primary_phase_148(a,state);
        }
    }
}
void primary_phase_146(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b404); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027b704); a.load<1>(4,15,0x105); a.move(3,23); a.load<1>(5,15,0x106); a.call(0x10281780);
        a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x1027b964);
            primary_phase_156(a,state);
        } else {
            a.at(0x1027b71c); a.floating_load(1,31,92,true); a.floating_load(0,31,0,true);
            a.floating_product_single(1,1,31); a.load<1>(13,15,0x10e); a.floating_compare(1,0);
            if (!a.equal()) {
                a.at(0x1027b73c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027b754); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027b764);
                primary_phase_152(a,state);
            } else {
                a.at(0x1027b734); a.value(14,0x0); a.jump(0x1027b938);
                primary_phase_155(a,state);
            }
        }
    } else {
        a.at(0x1027b40c); a.floating_load(1,31,228,true); a.floating_load(0,31,0,true);
        a.floating_product_single(1,1,31); a.load<1>(13,15,0x10e); a.floating_product_single(1,1,31);
        a.floating_compare(1,0);
        if (!a.equal()) {
            a.at(0x1027b430); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027b448); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027b458);
            primary_phase_147(a,state);
        } else {
            a.at(0x1027b428); a.value(14,0x0); a.jump(0x1027b64c);
            primary_phase_150(a,state);
        }
    }
}
void primary_phase_145(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b400); a.sign8(0,3);
    primary_phase_146(a,state);
}
void primary_phase_144(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b3d8); a.sign8(0,3); a.jump(0x1027b404);
    primary_phase_146(a,state);
}
void primary_phase_143(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b378); a.sign8(3,13); a.offset(0,3,0x5); a.compare_registers(0,14);
    if (a.positive()) {
        a.at(0x1027b388); a.floating_load(2,31,28,true); a.floating_load(1,15,152,true);
        a.floating_load(0,31,328,true);
        a.floating_product_difference(0,2,1,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.add(21,21,0); a.jump(0x1027b964);
    }
    a.at(0x1027b964);
    primary_phase_156(a,state);
}
void primary_phase_142(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b374); a.move(14,0);
    primary_phase_143(a,state);
}
void primary_phase_141(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b21c); a.floating_integer(1,31); a.value(3,0x43300000); a.floating_integer(0,31); a.load(8,30,0x0);
    a.value(7,0x80010000); a.value(4,0x80010000); a.floating_store(1,1,164); a.value(5,0x43300000);
    a.value(6,0x10000); a.floating_load(2,26,128); a.load(0,1,0xa8); a.offset(4,4,-0x7fff);
    a.floating_store(0,1,196); a.offset(7,7,-0x7fff); a.exclusive_flags(0,0,0x80000000); a.floating_load(0,26,0);
    a.store(1,0xb0,0); a.offset(6,6,-0x1); a.load(0,1,0xc8); a.store(1,0xac,3); a.multiply(3,0,0xa);
    a.load<2>(8,8,0x0); a.floating_load(1,1,172); a.load(0,27,0x0); a.floating_subtract(2,1,2);
    a.multiply_high(4,4,3); a.add(0,8,0); a.store(1,0xb4,5); a.floating_load(1,26,128);
    a.floating_difference(2,2,0,true); a.multiply_high(5,7,0); a.add(3,4,3); a.add(4,5,0); a.shift_right(4,4,0xf);
    a.mask(5,4,0x1,false,1); a.add(4,4,5); a.shift_right(3,3,0xf); a.product(5,4,6); a.mask(4,3,0x1,false,1);
    a.add(3,3,4); a.subtract(14,5,0); a.exclusive_flags(0,14,0x80000000); a.store(1,0xb8,0);
    a.floating_load(0,1,180); a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.floating_integer(0,0);
    a.floating_store(0,1,188); a.load(14,1,0xc0); a.call(0x10226520); a.value(4,0x66660000);
    a.floating_integer(0,31); a.offset(0,3,0x5); a.offset(4,4,0x6667); a.multiply_high(0,4,0);
    a.floating_store(0,1,204); a.load(3,1,0xd0); a.shift_right(4,0,0x2); a.offset(0,3,-0x1);
    a.mask(3,4,0x1,false,1); a.add(3,4,3); a.add(14,14,3); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027b31c); a.floating_integer(0,31); a.floating_store(0,1,212); a.load(3,1,0xd8);
        a.offset(14,3,-0x1); a.jump(0x1027b378);
    }
    a.at(0x1027b378);
    primary_phase_143(a,state);
}
void primary_phase_140(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b1e4); a.floating_integer(0,31); a.value(3,0x10000); a.offset(3,3,-0x1); a.floating_store(0,1,164);
    a.load(0,1,0xa8); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027b21c);
        primary_phase_141(a,state);
    } else {
        a.at(0x1027b200); a.floating_integer(0,31); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_store(0,1,164); a.load(0,1,0xa8); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027b330); a.floating_integer(1,31); a.load(3,30,0x0); a.floating_integer(0,31);
            a.load(0,27,0x0); a.load<2>(3,3,0x0); a.floating_store(1,1,164); a.add(5,3,0); a.load(4,1,0xa8);
            a.floating_store(0,1,172); a.divide(3,5,4); a.load(0,1,0xb0); a.compare(0,0); a.product(0,3,4);
            a.subtract(14,0,5); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027b370); a.move(0,14);
            }
            a.at(0x1027b374);
            primary_phase_142(a,state);
        } else {
            a.at(0x1027b21c);
            primary_phase_141(a,state);
        }
    }
}
void primary_phase_139(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b19c); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027b3ac); a.load<1>(0,23,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027b3e0); a.load<1>(0,23,0x105); a.value(3,0x0); a.compare(0,4);
            if (a.equal()) {
                a.at(0x1027b3f0); a.load<1>(0,23,0x106); a.sign8(0,0,true);
                if (!a.equal()) {
                } else {
                    a.at(0x1027b3fc); a.value(3,0x1);
                }
            }
            a.at(0x1027b400);
            primary_phase_145(a,state);
        } else {
            a.at(0x1027b3b8); a.load<1>(0,23,0x105); a.value(3,0x0); a.compare(0,4);
            if (a.equal()) {
                a.at(0x1027b3c8); a.load<1>(0,23,0x106); a.compare(0,11);
                if (!a.equal()) {
                } else {
                    a.at(0x1027b3d4); a.value(3,0x1);
                }
            }
            a.at(0x1027b3d8);
            primary_phase_144(a,state);
        }
    } else {
        a.at(0x1027b1a4); a.floating_load(0,31,0,true); a.load<1>(13,15,0x10e); a.floating_compare(31,0);
        if (!a.equal()) {
            a.at(0x1027b1bc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x1027b1d4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x1027b1e4);
            primary_phase_140(a,state);
        } else {
            a.at(0x1027b1b4); a.value(14,0x0); a.jump(0x1027b378);
            primary_phase_143(a,state);
        }
    }
}
void primary_phase_138(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b198); a.sign8(0,3);
    primary_phase_139(a,state);
}
void primary_phase_137(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b14c); a.sign8(0,3); a.jump(0x1027b19c);
    primary_phase_139(a,state);
}
void primary_phase_136(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b0e8); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027b154); a.load<1>(0,23,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027b168); a.load<1>(0,23,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027b178); a.load<1>(0,23,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x1027b184); a.load<1>(0,23,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027b194); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027b198);
        primary_phase_138(a,state);
    } else {
        a.at(0x1027b104); a.load<1>(0,23,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x1027b118); a.load<1>(0,23,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x1027b128); a.load<1>(0,23,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x1027b138); a.load<1>(0,23,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x1027b148); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x1027b14c);
        primary_phase_137(a,state);
    }
}
void primary_phase_135(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b0d4); a.sign8(3,13); a.offset(0,3,0x5); a.compare_registers(0,14);
    if (a.positive()) {
        a.at(0x1027b0e4); a.offset(21,21,0x5dc);
    }
    a.at(0x1027b0e8);
    primary_phase_136(a,state);
}
void primary_phase_134(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027b0d0); a.move(14,0);
    primary_phase_135(a,state);
}
void primary_phase_133(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027af48); a.floating_load(0,31,240,true); a.value(0,0x43300000); a.load(3,30,0x0);
    a.value(7,0x80010000); a.floating_product_single(1,0,31); a.floating_load(0,31,240,true); a.store(1,0xac,0);
    a.value(5,0x43300000); a.floating_product_single(0,0,31); a.value(6,0x10000); a.floating_integer(1,1);
    a.value(4,0x80010000); a.floating_integer(0,0); a.load<2>(8,3,0x0); a.load(0,27,0x0); a.offset(7,7,-0x7fff);
    a.floating_store(1,1,164); a.add(0,8,0); a.floating_load(3,26,128); a.multiply_high(7,7,0); a.load(3,1,0xa8);
    a.offset(6,6,-0x1); a.store(1,0xb4,5); a.offset(4,4,-0x7fff); a.exclusive_flags(3,3,0x80000000);
    a.store(1,0xb0,3); a.add(5,7,0); a.floating_load(2,26,0); a.shift_right(5,5,0xf); a.floating_store(0,1,196);
    a.mask(7,5,0x1,false,1); a.floating_load(0,1,172); a.add(5,5,7); a.product(5,5,6); a.floating_load(1,26,128);
    a.floating_subtract(0,0,3); a.load(3,1,0xc8); a.floating_difference(2,0,2,true); a.subtract(14,5,0);
    a.exclusive_flags(0,14,0x80000000); a.store(1,0xb8,0); a.multiply(0,3,0xa); a.floating_load(0,1,180);
    a.floating_subtract(0,0,1); a.multiply_high(3,4,0); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_integer(0,0); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.floating_store(0,1,188); a.load(14,1,0xc0); a.call(0x10226520); a.floating_load(0,31,240,true);
    a.value(4,0x66660000); a.offset(0,3,0x5); a.floating_product_single(0,0,31); a.offset(4,4,0x6667);
    a.multiply_high(0,4,0); a.floating_integer(0,0); a.floating_store(0,1,204); a.shift_right(0,0,0x2);
    a.mask(4,0,0x1,false,1); a.load(3,1,0xd0); a.add(0,0,4); a.add(14,14,0); a.offset(0,3,-0x1);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027b060); a.floating_load(0,31,240,true); a.floating_product_single(0,0,31);
        a.floating_integer(0,0); a.floating_store(0,1,212); a.load(3,1,0xd8); a.offset(14,3,-0x1);
        a.jump(0x1027b0d4);
    }
    a.at(0x1027b0d4);
    primary_phase_135(a,state);
}
void primary_phase_132(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027af00); a.floating_load(0,31,240,true); a.value(3,0x10000); a.offset(3,3,-0x1);
    a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1027af48);
        primary_phase_133(a,state);
    } else {
        a.at(0x1027af24); a.floating_load(0,31,240,true); a.value(3,0xffff0000); a.offset(3,3,0x1);
        a.floating_product_single(0,0,31); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x1027b07c); a.floating_load(0,31,240,true); a.load(3,30,0x0); a.floating_product_single(1,0,31);
            a.floating_load(0,31,240,true); a.load<2>(3,3,0x0); a.load(0,27,0x0);
            a.floating_product_single(0,0,31); a.floating_integer(1,1); a.add(5,3,0); a.floating_integer(0,0);
            a.floating_store(1,1,164); a.load(4,1,0xa8); a.floating_store(0,1,172); a.divide(3,5,4);
            a.load(0,1,0xb0); a.compare(0,0); a.product(0,3,4); a.subtract(14,0,5); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027b0cc); a.move(0,14);
            }
            a.at(0x1027b0d0);
            primary_phase_134(a,state);
        } else {
            a.at(0x1027af48);
            primary_phase_133(a,state);
        }
    }
}
void primary_phase_131(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae8c); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027b964);
        primary_phase_156(a,state);
    } else {
        a.at(0x1027ae94); a.move(3,23); a.call(0x10289560); a.mask(0,3,0x1,true,0);
        if (!a.equal()) {
            a.at(0x1027aeac); a.load<1>(0,23,0x2c); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x1027b0e8);
                primary_phase_136(a,state);
            } else {
                a.at(0x1027aeb8); a.floating_load(1,31,240,true); a.floating_load(0,31,0,true);
                a.floating_product_single(1,1,31); a.load<1>(13,15,0x10e); a.floating_compare(1,0);
                if (!a.equal()) {
                    a.at(0x1027aed8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027aef0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027af00);
                    primary_phase_132(a,state);
                } else {
                    a.at(0x1027aed0); a.value(14,0x0); a.jump(0x1027b0d4);
                    primary_phase_135(a,state);
                }
            }
        } else {
            a.at(0x1027aea4); a.offset(21,21,0x1388); a.jump(0x1027b0e8);
            primary_phase_136(a,state);
        }
    }
}
void primary_phase_130(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae88); a.value(0,0x0);
    primary_phase_131(a,state);
}
void primary_phase_129(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae80); a.value(0,0x1); a.jump(0x1027ae8c);
    primary_phase_131(a,state);
}
void primary_phase_128(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae64); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ae88);
        primary_phase_130(a,state);
    } else {
        a.at(0x1027ae70); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ae88);
            primary_phase_130(a,state);
        } else {
            a.at(0x1027ae80);
            primary_phase_129(a,state);
        }
    }
}
void primary_phase_127(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae40); a.compare(23,0,false);
    if (a.equal()) {
        a.at(0x1027b964);
        primary_phase_156(a,state);
    } else {
        a.at(0x1027ae48); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027ae64);
            primary_phase_128(a,state);
        } else {
            a.at(0x1027ae54); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x1027ae80);
                primary_phase_129(a,state);
            } else {
                a.at(0x1027ae64);
                primary_phase_128(a,state);
            }
        }
    }
}
void primary_phase_126(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae34); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ae3c); a.offset(21,21,-0x61a8);
    }
    a.at(0x1027ae40);
    primary_phase_127(a,state);
}
void primary_phase_125(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae30); a.value(0,0x0);
    primary_phase_126(a,state);
}
void primary_phase_124(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae28); a.value(0,0x1); a.jump(0x1027ae34);
    primary_phase_126(a,state);
}
void primary_phase_123(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ae0c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ae30);
        primary_phase_125(a,state);
    } else {
        a.at(0x1027ae18); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ae30);
            primary_phase_125(a,state);
        } else {
            a.at(0x1027ae28);
            primary_phase_124(a,state);
        }
    }
}
void primary_phase_122(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027adf0); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027ae0c);
        primary_phase_123(a,state);
    } else {
        a.at(0x1027adfc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ae28);
            primary_phase_124(a,state);
        } else {
            a.at(0x1027ae0c);
            primary_phase_123(a,state);
        }
    }
}
void primary_phase_121(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027adb0); a.move(3,15); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027ae40);
        primary_phase_127(a,state);
    } else {
        a.at(0x1027adc4); a.load<1>(0,15,0x10d); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1027add0); a.floating_load(2,31,248,true); a.floating_load(1,15,160,true);
            a.floating_load(0,31,344,true);
            a.floating_product_difference(0,2,1,0); { const auto rounded=-double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.floating_integer(0,0); a.floating_store(0,1,164); a.load(0,1,0xa8); a.subtract(19,0,19);
        }
        a.at(0x1027adf0);
        primary_phase_122(a,state);
    }
}
void primary_phase_120(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad80); a.floating_load(0,26,8); a.floating_compare(31,0);
    if (a.less()) {
        a.at(0x1027ad8c); a.compare(21,0);
        if (a.nonpositive()) {
        } else {
            a.at(0x1027ad94); a.value(3,0x66660000); a.offset(3,3,0x6667); a.multiply_high(0,3,21);
            a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.subtract(20,0,20);
        }
    }
    a.at(0x1027adb0);
    primary_phase_121(a,state);
}
void primary_phase_119(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad58); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ad60); a.load<2>(0,15,0x2a,true); a.compare(0,-20);
        if (a.nonpositive()) {
        } else {
            a.at(0x1027ad6c); a.compare(21,0);
            if (a.at_least()) {
            } else {
                a.at(0x1027ad74); a.value(0,0x1); a.offset(19,19,0xfa0); a.store<1>(1,0x102,0);
            }
        }
    }
    a.at(0x1027ad80);
    primary_phase_120(a,state);
}
void primary_phase_118(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad54); a.value(0,0x0);
    primary_phase_119(a,state);
}
void primary_phase_117(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad4c); a.value(0,0x1); a.jump(0x1027ad58);
    primary_phase_119(a,state);
}
void primary_phase_116(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad30); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ad54);
        primary_phase_118(a,state);
    } else {
        a.at(0x1027ad3c); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ad54);
            primary_phase_118(a,state);
        } else {
            a.at(0x1027ad4c);
            primary_phase_117(a,state);
        }
    }
}
void primary_phase_115(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ad14); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027ad30);
        primary_phase_116(a,state);
    } else {
        a.at(0x1027ad20); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027ad4c);
            primary_phase_117(a,state);
        } else {
            a.at(0x1027ad30);
            primary_phase_116(a,state);
        }
    }
}
void primary_phase_114(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ace0); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ad14);
        primary_phase_115(a,state);
    } else {
        a.at(0x1027ace8); a.move(3,15); a.value(4,0x1); a.call(0x10274170); a.value(0,0x10); a.offset(3,1,0x74);
        a.store<2>(15,0x19c,0); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13);
        a.jump(0x1028175c);
    }
}
void primary_phase_113(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027acdc); a.value(0,0x0);
    primary_phase_114(a,state);
}
void primary_phase_112(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027acd4); a.value(0,0x1); a.jump(0x1027ace0);
    primary_phase_114(a,state);
}
void primary_phase_111(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027acb8); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027acdc);
        primary_phase_113(a,state);
    } else {
        a.at(0x1027acc4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x1027acdc);
            primary_phase_113(a,state);
        } else {
            a.at(0x1027acd4);
            primary_phase_112(a,state);
        }
    }
}
void primary_phase_110(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ac94); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027ad14);
        primary_phase_115(a,state);
    } else {
        a.at(0x1027ac9c); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027acb8);
            primary_phase_111(a,state);
        } else {
            a.at(0x1027aca8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,3);
            if (a.less()) {
                a.at(0x1027acd4);
                primary_phase_112(a,state);
            } else {
                a.at(0x1027acb8);
                primary_phase_111(a,state);
            }
        }
    }
}
void primary_phase_109(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027ac88); a.value(0,0x1); a.jump(0x1027ac94);
    primary_phase_110(a,state);
}
void primary_phase_108(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027abe4); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,27,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0xa); a.subtract(0,0,4); a.compare_registers(13,0);
    if (a.at_least()) {
        a.at(0x1027ad80);
        primary_phase_120(a,state);
    } else {
        a.at(0x1027ac1c); a.load(3,15,0x1a4); a.load<1>(4,15,0x27); a.call(0x101fd000); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x1027ad80);
            primary_phase_120(a,state);
        } else {
            a.at(0x1027ac30); a.load(3,15,0x1a4); a.offset(5,1,0x4b); a.load<1>(4,15,0x27); a.offset(6,1,0x4a);
            a.offset(7,1,0x49); a.offset(8,1,0x48); a.offset(9,1,0x47); a.value(10,0x1); a.call(0x101f21d0);
            a.sign8(0,3,true);
            if (a.nonpositive()) {
                a.at(0x1027ad80);
                primary_phase_120(a,state);
            } else {
                a.at(0x1027ac5c); a.sign8(0,3); a.compare(0,3);
                if (a.at_least()) {
                    a.at(0x1027ad80);
                    primary_phase_120(a,state);
                } else {
                    a.at(0x1027ac68); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
                    if (a.less()) {
                        a.at(0x1027ac88);
                        primary_phase_109(a,state);
                    } else {
                        a.at(0x1027ac78); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                        if (a.nonpositive()) {
                            a.at(0x1027ac90); a.value(0,0x0);
                            primary_phase_110(a,state);
                        } else {
                            a.at(0x1027ac88);
                            primary_phase_109(a,state);
                        }
                    }
                }
            }
        }
    }
}
void primary_phase_107(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027abbc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x1027abd4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
    }
    a.at(0x1027abe4);
    primary_phase_108(a,state);
}
void primary_phase_106(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027abb8); a.move(13,0);
    primary_phase_107(a,state);
}
void primary_phase_105(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027aaa8); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027ab84); a.offset(13,14,-0x1); a.jump(0x1027abbc);
    }
    a.at(0x1027abbc);
    primary_phase_107(a,state);
}
void primary_phase_104(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027aa88); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027aaa8);
        primary_phase_105(a,state);
    } else {
        a.at(0x1027aa98); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027ab8c); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027abb4); a.move(0,13);
            }
            a.at(0x1027abb8);
            primary_phase_106(a,state);
        } else {
            a.at(0x1027aaa8);
            primary_phase_105(a,state);
        }
    }
}
void primary_phase_103(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027aa20); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ad80);
        primary_phase_120(a,state);
    } else {
        a.at(0x1027aa28); a.load<1>(0,15,0x109); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x1027ad80);
            primary_phase_120(a,state);
        } else {
            a.at(0x1027aa34); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,3);
            if (a.at_least()) {
                a.at(0x1027ad80);
                primary_phase_120(a,state);
            } else {
                a.at(0x1027aa48); a.load(3,15,0x6c); a.load<1>(14,3,0x68); a.sign8(14,14,true);
                if (!a.equal()) {
                    a.at(0x1027aa60); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x1027aa78); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x1027aa88);
                    primary_phase_104(a,state);
                } else {
                    a.at(0x1027aa58); a.value(13,0x0); a.jump(0x1027abbc);
                    primary_phase_107(a,state);
                }
            }
        }
    }
}
void primary_phase_102(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027aa1c); a.sign8(0,3);
    primary_phase_103(a,state);
}
void primary_phase_101(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a9d0); a.sign8(0,3); a.jump(0x1027aa20);
    primary_phase_103(a,state);
}
void primary_phase_100(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a914); a.load(6,30,0x0); a.value(4,0x66660000); a.value(3,0x2e8c0000); a.load(5,27,0x0);
    a.load<2>(6,6,0x0); a.offset(4,4,0x6667); a.sign16(0,13); a.offset(3,3,-0x5d17); a.add(6,6,5);
    a.multiply_high(4,4,6); a.shift_right(4,4,0x3); a.multiply_high(0,3,0); a.mask(5,4,0x1,false,1); a.add(3,4,5);
    a.multiply(3,3,0x14); a.shift_right(0,0,0x1); a.subtract(4,3,6); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.offset(0,4,0x46); a.compare_registers(3,0);
    if (a.nonpositive()) {
        a.at(0x1027ad80);
        primary_phase_120(a,state);
    } else {
        a.at(0x1027a96c); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027a9d8); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027a9ec); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027a9fc); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x1027aa08); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027aa18); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027aa1c);
            primary_phase_102(a,state);
        } else {
            a.at(0x1027a988); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x1027a99c); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x1027a9ac); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x1027a9bc); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x1027a9cc); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x1027a9d0);
            primary_phase_101(a,state);
        }
    }
}
void primary_phase_99(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a8c0); a.load<1>(0,1,0xff); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027ad80);
        primary_phase_120(a,state);
    } else {
        a.at(0x1027a8cc); a.load(0,1,0x68); a.compare(0,3000);
        if (a.at_least()) {
            a.at(0x1027ad80);
            primary_phase_120(a,state);
        } else {
            a.at(0x1027a8d8); a.sign8(0,16); a.compare(0,4);
            if (a.equal()) {
                a.at(0x1027ad80);
                primary_phase_120(a,state);
            } else {
                a.at(0x1027a8e4); a.load(4,15,0x1a4); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.load<2>(13,4,0x7948,true); a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027a904); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027a914);
                primary_phase_100(a,state);
            }
        }
    }
}
void primary_phase_98(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a878); a.subtract(3,14,13); a.value(0,0x43300000); a.exclusive_flags(3,3,0x80000000);
    a.store(1,0xa4,0); a.value(0,0x1); a.floating_load(3,31,236,true); a.store(1,0xa8,3);
    a.floating_load(2,31,340,true); a.floating_load(1,26,128); a.floating_load(0,1,164);
    a.floating_product_sum(2,3,31,2,true); a.store<1>(1,0xfc,0); a.floating_subtract(0,0,1,true);
    a.floating_product_single(0,2,0); a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0);
    a.add(19,19,0);
    primary_phase_99(a,state);
}
void primary_phase_97(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a874); a.move(14,0);
    primary_phase_98(a,state);
}
void primary_phase_96(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a764); a.exclusive_flags(3,18,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,18,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(14,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,18,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x1027a840); a.offset(14,18,-0x1); a.jump(0x1027a878);
    }
    a.at(0x1027a878);
    primary_phase_98(a,state);
}
void primary_phase_95(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a744); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(18,3);
    if (a.positive()) {
        a.at(0x1027a764);
        primary_phase_96(a,state);
    } else {
        a.at(0x1027a754); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(18,3);
        if (a.at_least()) {
            a.at(0x1027a848); a.load(3,30,0x0); a.compare(18,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,18); a.product(0,0,18); a.subtract(14,0,3); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027a870); a.move(0,14);
            }
            a.at(0x1027a874);
            primary_phase_97(a,state);
        } else {
            a.at(0x1027a764);
            primary_phase_96(a,state);
        }
    }
}
void primary_phase_94(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a6f4); a.floating_load(1,31,92,true); a.floating_load(0,15,156,true);
    a.floating_product_single(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,164); a.load(18,1,0xa8);
    a.compare(18,0);
    if (!a.equal()) {
        a.at(0x1027a71c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027a734); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027a744);
        primary_phase_95(a,state);
    } else {
        a.at(0x1027a714); a.value(14,0x0); a.jump(0x1027a878);
        primary_phase_98(a,state);
    }
}
void primary_phase_93(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a6f0); a.move(13,0);
    primary_phase_94(a,state);
}
void primary_phase_92(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a5e0); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027a6bc); a.offset(13,14,-0x1); a.jump(0x1027a6f4);
    }
    a.at(0x1027a6f4);
    primary_phase_94(a,state);
}
void primary_phase_91(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a5c0); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027a5e0);
        primary_phase_92(a,state);
    } else {
        a.at(0x1027a5d0); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027a6c4); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027a6ec); a.move(0,13);
            }
            a.at(0x1027a6f0);
            primary_phase_93(a,state);
        } else {
            a.at(0x1027a5e0);
            primary_phase_92(a,state);
        }
    }
}
void primary_phase_90(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a4d0); a.load<1>(3,23,0x105); a.load<1>(0,15,0x105); a.sign8(3,3); a.load<1>(13,23,0x106);
    a.sign8(0,0); a.subtract(3,3,0); a.call(0x10775540); a.load<1>(0,15,0x106); a.sign8(4,13); a.sign8(13,3);
    a.sign8(0,0); a.subtract(3,4,0); a.call(0x10775540); a.sign8(3,3); a.sign8(0,13); a.multiply(3,3,0x24);
    a.floating_load(0,26,136); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.load(3,2,-0x6424);
    a.floating_indexed_single(1,3,0); a.floating_compare(0,1);
    if (!a.equal()) {
        a.at(0x1027a8c0);
        primary_phase_99(a,state);
    } else {
        a.at(0x1027a52c); a.load<1>(0,15,0x105); a.load<1>(3,15,0x106); a.sign8(4,0); a.load<1>(0,15,0x27);
        a.sign8(3,3); a.load(5,15,0x1a4); a.multiply(4,4,0x450); a.sign8(0,0); a.leading_zeroes(0,0);
        a.multiply(3,3,0x5c); a.add(4,5,4); a.mask(0,0,0x7ffffff,false,27); a.add(3,4,3); a.add(3,3,0);
        a.load<1>(0,3,0xa60); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027a8c0);
            primary_phase_99(a,state);
        } else {
            a.at(0x1027a570); a.floating_load(1,31,92,true); a.floating_load(0,15,152,true);
            a.floating_product_single(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,164);
            a.load(14,1,0xa8); a.compare(14,0);
            if (!a.equal()) {
                a.at(0x1027a598); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027a5b0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027a5c0);
                primary_phase_91(a,state);
            } else {
                a.at(0x1027a590); a.value(13,0x0); a.jump(0x1027a6f4);
                primary_phase_94(a,state);
            }
        }
    }
}
void primary_phase_89(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a430); a.floating_load(1,15,152,true); a.value(3,0x43300000); a.floating_load(0,15,152,true);
    a.value(0,0x43300000); a.load(4,30,0x0); a.floating_product_single(2,1,0); a.floating_load(1,26,40);
    a.floating_load(0,26,40); a.load<2>(5,4,0x0); a.floating_product_sum(0,2,1,0); a.load(4,27,0x0);
    a.store(1,0xac,3); a.add(4,5,4); a.floating_load(3,26,128); a.mask(3,4,0xc0000000,false,30);
    a.floating_integer(0,0); a.mask(4,4,0x1,false,1); a.subtract(3,4,3); a.store(1,0xb4,0);
    a.mask(0,3,0xffffffff,false,2); a.floating_load(1,26,128); a.floating_store(0,1,164); a.add(4,0,4);
    a.offset(0,4,0x1); a.load(3,1,0xa8); a.exclusive_flags(0,0,0x80000000); a.store(1,0xb8,0); a.sign16(3,3);
    a.exclusive_flags(0,3,0x80000000); a.floating_load(0,1,180); a.store(1,0xb0,0); a.floating_subtract(0,0,1);
    a.floating_load(2,1,172); a.floating_subtract(1,2,3); a.floating_product(1,1,28); a.floating_product(0,1,0);
    a.floating_integer(0,0); a.floating_store(0,1,188); a.load(0,1,0xc0); a.add(19,19,0);
    primary_phase_90(a,state);
}
void primary_phase_88(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a3a4); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,4);
    if (a.at_least()) {
        a.at(0x1027a8c0);
        primary_phase_99(a,state);
    } else {
        a.at(0x1027a3b8); a.floating_load(0,31,0,true); a.floating_compare(31,0);
        if (a.nonpositive()) {
            a.at(0x1027a8c0);
            primary_phase_99(a,state);
        } else {
            a.at(0x1027a3c4); a.load<1>(0,1,0xff); a.compare(0,2);
            if (!a.equal()) {
                a.at(0x1027a4d0);
                primary_phase_90(a,state);
            } else {
                a.at(0x1027a3d0); a.floating_integer(0,31); a.value(0,0x43300000); a.store(1,0xac,0);
                a.floating_load(1,26,128); a.floating_store(0,1,164); a.floating_load(2,26,8); a.load(0,1,0xa8);
                a.exclusive_flags(0,0,0x80000000); a.store(1,0xb0,0); a.floating_load(0,1,172);
                a.floating_subtract(1,0,1); a.call(0x10780fc0); a.load(2,1,0x14); a.load(3,30,0x0);
                a.floating_move(28,1); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
                a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027a420); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027a430);
                primary_phase_89(a,state);
            }
        }
    }
}
void primary_phase_87(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a344); a.compare(13,1);
    if (a.less()) {
        a.at(0x1027a34c); a.load<1>(3,15,0x103); a.offset(0,3,-0x1); a.store<1>(15,0x103,0); a.jump(0x1027a3a4);
    }
    a.at(0x1027a3a4);
    primary_phase_88(a,state);
}
void primary_phase_86(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a340); a.move(13,0);
    primary_phase_87(a,state);
}
void primary_phase_85(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a230); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xa8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xa4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,164); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,14,0xa); a.offset(3,3,-0x7fff); a.store(1,0xac,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(13,4,7);
    a.exclusive_flags(4,13,0x80000000); a.store(1,0xb0,4); a.multiply_high(3,3,0); a.floating_load(0,1,172);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,180); a.load(13,1,0xb8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,14,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(13,13,3);
    a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x1027a30c); a.offset(13,14,-0x1); a.jump(0x1027a344);
    }
    a.at(0x1027a344);
    primary_phase_87(a,state);
}
void primary_phase_84(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a210); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(14,3);
    if (a.positive()) {
        a.at(0x1027a230);
        primary_phase_85(a,state);
    } else {
        a.at(0x1027a220); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(14,3);
        if (a.at_least()) {
            a.at(0x1027a314); a.load(3,30,0x0); a.compare(14,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,14); a.product(0,0,14); a.subtract(13,0,3); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027a33c); a.move(0,13);
            }
            a.at(0x1027a340);
            primary_phase_86(a,state);
        } else {
            a.at(0x1027a230);
            primary_phase_85(a,state);
        }
    }
}
void primary_phase_83(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a158); a.load<1>(0,15,0x27); a.load(3,15,0x1a4); a.sign8(0,0); a.add(3,3,0); a.load<1>(0,3,0x1c4);
    a.compare(0,5);
    if (!a.equal()) {
        a.at(0x1027a35c); a.load<1>(3,1,0x4b); a.load<1>(0,1,0x4a); a.sign8(3,3); a.offset(3,3,-0x2); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.positive()) {
            a.at(0x1027a378); a.load<1>(0,1,0x4b); a.load<1>(4,1,0x4a); a.sign8(3,0); a.load(0,1,0x68);
            a.sign8(4,4); a.offset(3,3,-0x2); a.mask(0,0,0xfffffffe,false,1); a.subtract(3,4,3); a.store(1,0x68,0);
            a.multiply(0,3,0xbb8); a.subtract(21,0,21);
        }
        a.at(0x1027a3a4);
        primary_phase_88(a,state);
    } else {
        a.at(0x1027a174); a.load<1>(0,1,0x4a); a.move(3,15); a.load<1>(4,1,0x4b); a.sign8(5,0); a.load(0,1,0x68);
        a.sign8(4,4); a.subtract(4,5,4); a.mask(0,0,0xfffffffe,false,1); a.multiply(4,4,0xbb8); a.store(1,0x68,0);
        a.subtract(21,4,21); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
        if (a.equal()) {
            a.at(0x1027a3a4);
            primary_phase_88(a,state);
        } else {
            a.at(0x1027a1b0); a.load<1>(4,1,0x4a); a.load<1>(0,1,0x4b); a.load(3,15,0x6c); a.sign8(4,4);
            a.sign8(0,0); a.subtract(4,4,0); a.load<1>(0,3,0x67); a.multiply(3,4,0x6); a.sign8(0,0);
            a.mask(0,0,0xfffffffe,false,1); a.subtract(14,3,0,true);
            if (!a.equal()) {
                a.at(0x1027a1e8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x1027a200); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x1027a210);
                primary_phase_84(a,state);
            } else {
                a.at(0x1027a1e0); a.value(13,0x0); a.jump(0x1027a344);
                primary_phase_87(a,state);
            }
        }
    }
}
void primary_phase_82(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a154); a.value(21,0xfffffe0c);
    primary_phase_83(a,state);
}
void primary_phase_81(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a0a0); a.product(3,21,21); a.value(0,0x43300000); a.exclusive_flags(5,4,0x80000000);
    a.store(1,0xa4,0); a.value(4,0x43300000); a.floating_load(3,26,128); a.exclusive_flags(3,3,0x80000000);
    a.sign8(0,17); a.store(1,0xa8,3); a.multiply(3,0,0x2bc); a.value(0,0x43300000); a.floating_load(4,26,8);
    a.floating_load(2,1,164); a.floating_product_single(0,31,31); a.floating_load(1,31,336,true);
    a.floating_subtract(5,2,3); a.exclusive_flags(3,3,0x80000000); a.store(1,0xb0,5);
    a.floating_product_single(3,1,0); a.floating_load(2,26,128); a.floating_difference(4,5,4,true);
    a.store(1,0xac,4); a.floating_load(1,26,128); a.floating_load(0,1,172); a.store(1,0xb8,3); a.store(1,0xb4,0);
    a.floating_subtract(2,0,2,true); a.floating_load(0,1,180); a.floating_subtract(0,0,1);
    a.floating_difference(1,3,2,true,true); a.floating_subtract(1,4,1); a.floating_subtract(0,1,0);
    a.floating_integer(0,0); a.floating_store(0,1,188); a.load(21,1,0xc0); a.jump(0x1027a158);
    primary_phase_83(a,state);
}
void primary_phase_80(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a068); a.add(0,21,14); a.compare(21,0); a.store(1,0x104,0); a.value(0,0x1); a.store<1>(1,0x101,0);
    if (a.nonpositive()) {
        a.at(0x1027a130); a.load<1>(0,15,0x10e); a.load<1>(3,15,0x109); a.sign8(0,0); a.sign8(3,3); a.add(0,3,0);
        a.mask(3,0,0xfffffffc,false,2); a.offset(0,3,0x2); a.product(21,21,0); a.jump(0x1027a158);
        primary_phase_83(a,state);
    } else {
        a.at(0x1027a080); a.load<1>(0,15,0x10c); a.sign8(0,0); a.compare(0,1);
        if (a.at_least()) {
            a.at(0x1027a098); a.load<1>(0,15,0x10c); a.sign8(4,0);
        } else {
            a.at(0x1027a090); a.value(4,0x1); a.jump(0x1027a0a0);
        }
        a.at(0x1027a0a0);
        primary_phase_81(a,state);
    }
}
void primary_phase_79(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027a064); a.move(14,0);
    primary_phase_80(a,state);
}
void primary_phase_78(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279df0); a.sign8(0,13); a.value(3,0x43300000); a.mask(0,0,0xfffffffc,false,2); a.store(1,0xa4,3);
    a.exclusive_flags(0,0,0x80000000); a.load(5,30,0x0); a.store(1,0xa8,0); a.sign8(0,13);
    a.mask(0,0,0xfffffffc,false,2); a.value(4,0x80010000); a.floating_load(1,26,128);
    a.exclusive_flags(7,0,0x80000000); a.floating_load(0,1,164); a.value(6,0x43300000); a.value(10,0x43300000);
    a.value(3,0x10000); a.floating_subtract(0,0,1,true); a.floating_load(2,31,144,true); a.value(9,0x43300000);
    a.value(8,0x80010000); a.load<2>(5,5,0x0); a.offset(4,4,-0x7fff);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.load(0,27,0x0); a.store(1,0xd0,7); a.offset(3,3,-0x1); a.add(0,5,0); a.floating_load(1,26,128);
    a.floating_integer(0,0); a.multiply_high(5,4,0); a.store(1,0xcc,6); a.offset(8,8,-0x7fff);
    a.floating_load(4,26,128); a.floating_store(0,1,172); a.floating_load(0,1,204); a.load(4,1,0xb0);
    a.floating_subtract(0,0,1,true); a.store(1,0xb4,10); a.add(5,5,0); a.subtract(4,18,4);
    a.floating_load(2,31,144,true); a.exclusive_flags(4,4,0x80000000); a.store(1,0xb8,4);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_load(3,26,0); a.shift_right(4,5,0xf); a.floating_load(1,1,180); a.mask(5,4,0x1,false,1);
    a.add(4,4,5); a.floating_subtract(1,1,4); a.product(3,4,3); a.floating_integer(0,0); a.store(1,0xbc,9);
    a.floating_load(2,26,128); a.floating_difference(3,1,3,true); a.floating_store(0,1,212); a.subtract(14,3,0);
    a.exclusive_flags(0,14,0x80000000); a.store(1,0xc0,0); a.load(0,1,0xd8); a.floating_load(1,1,188);
    a.subtract(0,18,0); a.multiply(0,0,0xa); a.floating_subtract(1,1,2); a.multiply_high(3,8,0);
    a.floating_product(0,3,1); a.floating_integer(0,0); a.add(0,3,0); a.shift_right(0,0,0xf);
    a.mask(3,0,0x1,false,1); a.floating_store(0,1,196); a.add(3,0,3); a.load(14,1,0xc8); a.call(0x10226520);
    a.sign8(4,13); a.value(0,0x43300000); a.mask(4,4,0xfffffffc,false,2); a.value(5,0x66660000);
    a.exclusive_flags(4,4,0x80000000); a.store(1,0xdc,0); a.floating_load(1,26,128); a.offset(3,3,0x5);
    a.store(1,0xe0,4); a.offset(5,5,0x6667); a.multiply_high(0,5,3); a.floating_load(2,31,144,true);
    a.floating_load(0,1,220); a.floating_subtract(0,0,1,true); a.shift_right(0,0,0x2);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(14,14,0); a.floating_integer(0,0); a.floating_store(0,1,228);
    a.load(0,1,0xe8); a.subtract(3,18,0); a.offset(0,3,-0x1); a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10279f80); a.sign8(3,13); a.value(0,0x43300000); a.mask(3,3,0xfffffffc,false,2); a.store(1,0xec,0);
        a.exclusive_flags(0,3,0x80000000); a.floating_load(1,26,128); a.store(1,0xf0,0);
        a.floating_load(2,31,144,true); a.floating_load(0,1,236); a.floating_subtract(0,0,1,true);
        a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,244); a.load(0,1,0xf8); a.subtract(3,18,0);
        a.offset(14,3,-0x1); a.jump(0x1027a068);
    }
    a.at(0x1027a068);
    primary_phase_80(a,state);
}
void primary_phase_77(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279d58); a.sign8(3,13); a.value(0,0x43300000); a.mask(4,3,0xfffffffc,false,2); a.store(1,0xa4,0);
    a.exclusive_flags(4,4,0x80000000); a.value(3,0x10000); a.store(1,0xa8,4); a.offset(3,3,-0x1);
    a.floating_load(1,26,128); a.floating_load(0,1,164); a.floating_load(2,31,144,true);
    a.floating_subtract(0,0,1,true);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.subtract(0,18,0);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10279df0);
        primary_phase_78(a,state);
    } else {
        a.at(0x10279da4); a.sign8(3,13); a.value(0,0x43300000); a.mask(4,3,0xfffffffc,false,2); a.store(1,0xa4,0);
        a.exclusive_flags(4,4,0x80000000); a.value(3,0xffff0000); a.store(1,0xa8,4); a.offset(3,3,0x1);
        a.floating_load(1,26,128); a.floating_load(0,1,164); a.floating_load(2,31,144,true);
        a.floating_subtract(0,0,1,true);
        a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.subtract(0,18,0);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10279fc4); a.sign8(0,13); a.value(4,0x43300000); a.mask(0,0,0xfffffffc,false,2);
            a.load(5,30,0x0); a.exclusive_flags(3,0,0x80000000); a.sign8(0,13); a.store(1,0xa8,3);
            a.mask(0,0,0xfffffffc,false,2); a.exclusive_flags(3,0,0x80000000); a.value(0,0x43300000);
            a.store(1,0xa4,4); a.floating_load(1,26,128); a.floating_load(0,1,164); a.floating_load(2,31,144,true);
            a.floating_subtract(0,0,1,true); a.load<2>(5,5,0x0); a.load(4,27,0x0); a.store(1,0xb8,3);
            a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.add(5,5,4); a.store(1,0xb4,0); a.floating_load(1,26,128); a.floating_integer(3,0);
            a.floating_load(0,1,180); a.floating_load(2,31,144,true); a.floating_subtract(0,0,1,true);
            a.floating_store(3,1,172); a.load(0,1,0xb0);
            a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.subtract(4,18,0); a.divide(3,5,4); a.floating_integer(0,0); a.floating_store(0,1,188);
            a.load(0,1,0xc0); a.subtract(0,18,0,true); a.product(0,3,4); a.subtract(14,0,5); a.negate(0,14);
            if (a.positive()) {
                a.at(0x1027a060); a.move(0,14);
            }
            a.at(0x1027a064);
            primary_phase_79(a,state);
        } else {
            a.at(0x10279df0);
            primary_phase_78(a,state);
        }
    }
}
void primary_phase_76(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279ce8); a.sign8(3,13); a.value(0,0x43300000); a.mask(3,3,0xfffffffc,false,2); a.store(1,0xa4,0);
    a.exclusive_flags(0,3,0x80000000); a.floating_load(1,26,128); a.store(1,0xa8,0);
    a.floating_load(2,31,144,true); a.floating_load(0,1,164); a.floating_subtract(0,0,1,true);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,172); a.load(0,1,0xb0); a.subtract(0,18,0,true);
    if (!a.equal()) {
        a.at(0x10279d30); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10279d48); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10279d58);
        primary_phase_77(a,state);
    } else {
        a.at(0x10279d28); a.value(14,0x0); a.jump(0x1027a068);
        primary_phase_80(a,state);
    }
}
void primary_phase_75(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279ce4); a.move(18,0);
    primary_phase_76(a,state);
}
void primary_phase_74(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279a84); a.sign8(0,14); a.value(6,0x43300000); a.mask(0,0,0xfffffffc,false,2); a.load(4,30,0x0);
    a.exclusive_flags(0,0,0x80000000); a.value(3,0x80010000); a.store(1,0xe8,0); a.sign8(0,14);
    a.value(9,0x43300000); a.value(5,0x43300000); a.store(1,0xe4,6); a.mask(0,0,0xfffffffc,false,2);
    a.floating_load(1,26,128); a.exclusive_flags(6,0,0x80000000); a.floating_load(0,1,228); a.value(10,0x10000);
    a.floating_load(2,31,144,true); a.value(8,0x43300000); a.floating_subtract(0,0,1,true); a.value(7,0x80010000);
    a.load<2>(4,4,0x0); a.offset(3,3,-0x7fff); a.load(0,27,0x0); a.offset(10,10,-0x1);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.add(0,4,0); a.multiply_high(4,3,0); a.store(1,0xf4,9); a.floating_load(5,26,128); a.offset(7,7,-0x7fff);
    a.floating_integer(0,0); a.store(1,0xd0,6); a.floating_store(0,1,236); a.add(4,4,0); a.floating_load(1,26,128);
    a.load(3,1,0xf0); a.store(1,0xcc,5); a.exclusive_flags(3,3,0x80000000); a.floating_load(4,26,0);
    a.store(1,0xf8,3); a.shift_right(3,4,0xf); a.floating_load(0,1,204); a.mask(4,3,0x1,false,1);
    a.floating_load(3,1,244); a.add(3,3,4); a.floating_subtract(0,0,1,true); a.floating_load(2,31,144,true);
    a.floating_subtract(1,3,5); a.product(3,3,10); a.store(1,0xdc,8);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_difference(2,1,4,true); a.floating_load(3,26,128); a.subtract(18,3,0);
    a.exclusive_flags(0,18,0x80000000); a.store(1,0xe0,0); a.floating_load(1,1,220); a.floating_integer(0,0);
    a.floating_subtract(1,1,3); a.floating_store(0,1,196); a.floating_product(0,2,1); a.load(0,1,0xc8);
    a.multiply(0,0,0xa); a.floating_integer(0,0); a.multiply_high(3,7,0); a.floating_store(0,1,212);
    a.load(18,1,0xd8); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(3,0,3);
    a.call(0x10226520); a.sign8(4,14); a.value(0,0x43300000); a.mask(4,4,0xfffffffc,false,2);
    a.value(5,0x66660000); a.exclusive_flags(4,4,0x80000000); a.store(1,0xbc,0); a.floating_load(1,26,128);
    a.offset(3,3,0x5); a.store(1,0xc0,4); a.offset(5,5,0x6667); a.multiply_high(0,5,3);
    a.floating_load(2,31,144,true); a.floating_load(0,1,188); a.floating_subtract(0,0,1,true);
    a.shift_right(0,0,0x2);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(18,18,0); a.floating_integer(0,0); a.floating_store(0,1,180);
    a.load(3,1,0xb8); a.offset(0,3,-0x1); a.compare_registers(18,0);
    if (a.positive()) {
        a.at(0x10279c08); a.sign8(3,14); a.value(0,0x43300000); a.mask(3,3,0xfffffffc,false,2); a.store(1,0xac,0);
        a.exclusive_flags(0,3,0x80000000); a.floating_load(1,26,128); a.store(1,0xb0,0);
        a.floating_load(2,31,144,true); a.floating_load(0,1,172); a.floating_subtract(0,0,1,true);
        a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,164); a.load(3,1,0xa8); a.offset(18,3,-0x1);
        a.jump(0x10279ce8);
    }
    a.at(0x10279ce8);
    primary_phase_76(a,state);
}
void primary_phase_73(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102799f4); a.sign8(3,14); a.value(0,0x43300000); a.mask(4,3,0xfffffffc,false,2); a.store(1,0xe4,0);
    a.exclusive_flags(4,4,0x80000000); a.value(3,0x10000); a.store(1,0xe8,4); a.offset(3,3,-0x1);
    a.floating_load(1,26,128); a.floating_load(0,1,228); a.floating_load(2,31,144,true);
    a.floating_subtract(0,0,1,true);
    a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_integer(0,0); a.floating_store(0,1,236); a.load(0,1,0xf0); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10279a84);
        primary_phase_74(a,state);
    } else {
        a.at(0x10279a3c); a.sign8(3,14); a.value(0,0x43300000); a.mask(4,3,0xfffffffc,false,2); a.store(1,0xe4,0);
        a.exclusive_flags(4,4,0x80000000); a.value(3,0xffff0000); a.store(1,0xe8,4); a.offset(3,3,0x1);
        a.floating_load(1,26,128); a.floating_load(0,1,228); a.floating_load(2,31,144,true);
        a.floating_subtract(0,0,1,true);
        a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
        a.floating_integer(0,0); a.floating_store(0,1,236); a.load(0,1,0xf0); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10279c48); a.sign8(0,14); a.value(4,0x43300000); a.mask(0,0,0xfffffffc,false,2);
            a.load(5,30,0x0); a.exclusive_flags(3,0,0x80000000); a.sign8(0,14); a.store(1,0xa8,3);
            a.mask(0,0,0xfffffffc,false,2); a.exclusive_flags(3,0,0x80000000); a.value(0,0x43300000);
            a.store(1,0xa4,4); a.floating_load(1,26,128); a.floating_load(0,1,164); a.floating_load(2,31,144,true);
            a.floating_subtract(0,0,1,true); a.load<2>(5,5,0x0); a.load(4,27,0x0); a.store(1,0xb8,3);
            a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.add(5,5,4); a.store(1,0xb4,0); a.floating_load(1,26,128); a.floating_integer(3,0);
            a.floating_load(0,1,180); a.floating_load(2,31,144,true); a.floating_subtract(0,0,1,true);
            a.floating_store(3,1,172); a.load(4,1,0xb0);
            a.floating_product_difference(0,2,31,0); { const auto rounded=double(float(float_value<8>(state.r<8>(16384)))); state.wr<8>(16384,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
            a.divide(3,5,4); a.floating_integer(0,0); a.floating_store(0,1,188); a.load(0,1,0xc0); a.compare(0,0);
            a.product(0,3,4); a.subtract(18,0,5); a.negate(0,18);
            if (a.positive()) {
                a.at(0x10279ce0); a.move(0,18);
            }
            a.at(0x10279ce4);
            primary_phase_75(a,state);
        } else {
            a.at(0x10279a84);
            primary_phase_74(a,state);
        }
    }
}
void primary_phase_72(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279970); a.move(3,15); a.offset(4,1,0x51); a.offset(5,1,0x5c); a.call(0x10252080);
    a.load<1>(14,15,0x109); a.value(0,0x43300000); a.store(1,0xe4,0); a.sign16(21,3); a.sign8(0,14);
    a.floating_load(2,26,128); a.mask(0,0,0xfffffffc,false,2); a.floating_load(3,31,144,true);
    a.exclusive_flags(0,0,0x80000000); a.floating_load(0,31,0,true); a.store(1,0xe8,0); a.load<1>(13,15,0x109);
    a.floating_load(1,1,228); a.floating_subtract(1,1,2,true);
    a.floating_product_difference(1,3,31,1); { const auto rounded=double(float(float_value<8>(state.r<8>(16400)))); state.wr<8>(16400,float_bits<8>(rounded)); a.floating_result_status(rounded,false,true); }
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x102799cc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x102799e4); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x102799f4);
        primary_phase_73(a,state);
    } else {
        a.at(0x102799c4); a.value(18,0x0); a.jump(0x10279ce8);
        primary_phase_76(a,state);
    }
}
void primary_phase_71(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279934); a.load(5,30,0x0); a.value(3,0x51ec0000); a.load(4,27,0x0); a.offset(3,3,-0x7ae1);
    a.load<2>(5,5,0x0); a.sign8(0,13); a.add(5,5,4); a.multiply_high(3,3,5); a.shift_right(3,3,0x4);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x32); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.nonpositive()) {
        a.at(0x1027a154);
        primary_phase_82(a,state);
    } else {
        a.at(0x10279970);
        primary_phase_72(a,state);
    }
}
void primary_phase_70(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279884); a.load(3,30,0x0); a.value(5,0x55550000); a.value(4,0x92490000); a.value(0,0x43300000);
    a.load<2>(7,3,0x0); a.offset(5,5,0x5556); a.load(6,27,0x0); a.sign8(3,13); a.offset(4,4,0x2493);
    a.store(1,0xe4,0); a.add(6,7,6); a.floating_load(1,26,128); a.multiply_high(5,5,6); a.multiply_high(0,4,3);
    a.mask(4,5,0x1,false,1); a.add(4,5,4); a.multiply(4,4,0x3); a.add(0,0,3); a.shift_right(0,0,0x2);
    a.mask(3,0,0x1,false,1); a.subtract(4,4,6); a.add(0,0,3); a.add(3,0,4); a.offset(0,3,0x3);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0xe8,0); a.floating_load(0,1,228);
    a.floating_subtract(0,0,1,true); a.floating_compare(31,0);
    if (a.at_least()) {
        a.at(0x1027a154);
        primary_phase_82(a,state);
    } else {
        a.at(0x102798fc); a.sign8(0,16); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x10279970);
            primary_phase_72(a,state);
        } else {
            a.at(0x10279908); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.load<1>(13,15,0x10c);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10279924); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x10279934);
            primary_phase_71(a,state);
        }
    }
}
void primary_phase_69(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102797d0); a.value(21,0xffffec78); a.jump(0x1027a158);
    primary_phase_83(a,state);
}
void primary_phase_68(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279788); a.sign8(3,17); a.value(0,0x43300000); a.multiply(3,3,0x190); a.store(1,0xe4,0);
    a.floating_load(3,31,144,true); a.floating_load(2,15,172,true); a.exclusive_flags(0,3,0x80000000);
    a.floating_load(0,31,328,true); a.store(1,0xe8,0); a.floating_load(1,26,128);
    a.floating_product_sum(2,3,2,0,true); a.floating_load(0,1,228); a.floating_subtract(0,0,1,true);
    a.floating_subtract(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,236); a.load(21,1,0xf0);
    a.jump(0x1027a158);
    primary_phase_83(a,state);
}
void primary_phase_67(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279770); a.load<1>(0,15,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540); a.compare(3,1);
    if (!a.equal()) {
        a.at(0x102797d0);
        primary_phase_69(a,state);
    } else {
        a.at(0x10279788);
        primary_phase_68(a,state);
    }
}
void primary_phase_66(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027973c); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.compare(0,4);
    if (!a.equal()) {
        a.at(0x102797d8); a.floating_load(0,26,144); a.floating_compare(31,0); a.compare_less_or_equal_bit();
        if (!a.equal()) {
            a.at(0x10279858); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.load<1>(13,15,0x10c);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10279874); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x10279884);
            primary_phase_70(a,state);
        } else {
            a.at(0x102797e8); a.sign8(3,17); a.value(0,0x43300000); a.multiply(3,3,0x190);
            a.load<2>(5,15,0x2a,true); a.value(4,0x43300000); a.store(1,0xec,0); a.exclusive_flags(5,5,0x80000000);
            a.floating_load(4,31,28,true); a.exclusive_flags(0,3,0x80000000); a.floating_load(3,15,156,true);
            a.store(1,0xf0,0); a.floating_load(2,31,332,true); a.floating_load(1,26,128); a.floating_load(0,1,236);
            a.floating_product_sum(2,4,3,2,true); a.store(1,0xe8,5); a.floating_subtract(0,0,1,true);
            a.floating_load(3,26,128); a.store(1,0xe4,4); a.floating_load(4,26,400); a.floating_load(1,1,228);
            a.floating_subtract(0,2,0,true); a.floating_subtract(1,1,3); a.floating_product_sum(0,4,1,0);
            a.floating_integer(0,0); a.floating_store(0,1,244); a.load(21,1,0xf8); a.jump(0x1027a158);
            primary_phase_83(a,state);
        }
    } else {
        a.at(0x1027974c); a.floating_load(0,26,16); a.floating_compare(31,0);
        if (a.less()) {
            a.at(0x10279788);
            primary_phase_68(a,state);
        } else {
            a.at(0x10279758); a.load<1>(0,15,0x106); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x10279770);
                primary_phase_67(a,state);
            } else {
                a.at(0x10279764); a.load<1>(0,15,0x106); a.compare(0,11);
                if (!a.equal()) {
                    a.at(0x102797d0);
                    primary_phase_69(a,state);
                } else {
                    a.at(0x10279770);
                    primary_phase_67(a,state);
                }
            }
        }
    }
}
void primary_phase_65(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279710); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10279718); a.load(3,1,0x68); a.load(4,1,0x68); a.mask(0,3,0x1,false,1); a.subtract_from(4,4,0xfa0);
        a.add(0,0,3); a.shift_right(0,0,0x1); a.add(19,19,4); a.subtract_from(0,0,0x7d0); a.add(20,20,0);
    }
    a.at(0x1027973c);
    primary_phase_66(a,state);
}
void primary_phase_64(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027970c); a.value(0,0x0);
    primary_phase_65(a,state);
}
void primary_phase_63(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279704); a.value(0,0x1); a.jump(0x10279710);
    primary_phase_65(a,state);
}
void primary_phase_62(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102796e8); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027970c);
        primary_phase_64(a,state);
    } else {
        a.at(0x102796f4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x1027970c);
            primary_phase_64(a,state);
        } else {
            a.at(0x10279704);
            primary_phase_63(a,state);
        }
    }
}
void primary_phase_61(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102796c4); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027973c);
        primary_phase_66(a,state);
    } else {
        a.at(0x102796cc); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x102796e8);
            primary_phase_62(a,state);
        } else {
            a.at(0x102796d8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x10279704);
                primary_phase_63(a,state);
            } else {
                a.at(0x102796e8);
                primary_phase_62(a,state);
            }
        }
    }
}
void primary_phase_60(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102796c0); a.sign8(0,3);
    primary_phase_61(a,state);
}
void primary_phase_59(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279674); a.sign8(0,3); a.jump(0x102796c4);
    primary_phase_61(a,state);
}
void primary_phase_58(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279618); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027973c);
        primary_phase_66(a,state);
    } else {
        a.at(0x10279620); a.load<1>(0,15,0x27); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x1027967c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x10279690); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x102796a0); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x102796ac); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x102796bc); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x102796c0);
            primary_phase_60(a,state);
        } else {
            a.at(0x1027962c); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x10279640); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x10279650); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x10279660); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x10279670); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x10279674);
            primary_phase_59(a,state);
        }
    }
}
void primary_phase_57(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279614); a.sign8(0,3);
    primary_phase_58(a,state);
}
void primary_phase_56(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102795c8); a.sign8(0,3); a.jump(0x10279618);
    primary_phase_58(a,state);
}
void primary_phase_55(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279528); a.load(5,30,0x0); a.value(3,0x66660000); a.load(4,27,0x0); a.offset(3,3,0x6667);
    a.load<2>(5,5,0x0); a.sign8(0,13); a.add(5,5,4); a.multiply_high(3,3,5); a.shift_right(3,3,0x1);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x5); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.nonpositive()) {
        a.at(0x1027973c);
        primary_phase_66(a,state);
    } else {
        a.at(0x10279564); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x102795d0); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x102795e4); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x102795f4); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                    if (a.less()) {
                    } else {
                        a.at(0x10279600); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                        if (a.positive()) {
                        } else {
                            a.at(0x10279610); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x10279614);
            primary_phase_57(a,state);
        } else {
            a.at(0x10279580); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
            if (a.at_least()) {
                a.at(0x10279594); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                if (a.positive()) {
                } else {
                    a.at(0x102795a4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                    if (a.less()) {
                    } else {
                        a.at(0x102795b4); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                        if (a.positive()) {
                        } else {
                            a.at(0x102795c4); a.value(3,0x1);
                        }
                    }
                }
            }
            a.at(0x102795c8);
            primary_phase_56(a,state);
        }
    }
}
void primary_phase_54(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102794c0); a.load<2>(3,15,0x2a,true); a.offset(0,3,0x46); a.store<2>(15,0x2a,0); a.load<1>(0,15,0x27);
    a.load(3,15,0x1a4); a.sign8(0,0); a.leading_zeroes(0,0); a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.multiply(0,0,0x18e0); a.add(3,3,0); a.load(0,3,-0x7e0c); a.mask(0,0,0x400,true,0);
    if (a.equal()) {
        a.at(0x1027973c);
        primary_phase_66(a,state);
    } else {
        a.at(0x102794fc); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.load<1>(13,15,0x109);
        a.compare_registers(3,0,false); a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10279518); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10279528);
        primary_phase_55(a,state);
    }
}
void primary_phase_53(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102793d8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1027944c); a.value(0,0xffffffff); a.offset(6,1,0x50); a.store(1,0x38,0); a.offset(7,1,0x56);
        a.offset(8,1,0x4f); a.offset(9,1,0x4e); a.load(3,15,0x1a4); a.offset(10,1,0x60); a.load<1>(4,15,0x105);
        a.load<1>(5,15,0x106); a.call(0x101e8950); a.load<1>(0,1,0xfd); a.value(5,0x66660000); a.sign8(4,17);
        a.load(6,1,0x60); a.sign8(0,0); a.offset(5,5,0x6667); a.add(4,4,0); a.move(22,3); a.offset(0,4,0x4);
        a.product(0,0,6); a.multiply_high(0,5,0); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3);
        a.store(1,0x68,0); a.jump(0x102794c0);
    } else {
        a.at(0x102793e0); a.value(0,0xffffffff); a.offset(6,1,0x50); a.store(1,0x38,0); a.offset(7,1,0x56);
        a.offset(8,1,0x4f); a.offset(9,1,0x4e); a.load(3,15,0x1a4); a.offset(10,1,0x64); a.load<1>(4,15,0x105);
        a.load<1>(5,15,0x106); a.call(0x101e8950); a.load<1>(0,1,0xfd); a.value(5,0x66660000); a.sign8(4,17);
        a.load(6,1,0x64); a.sign8(0,0); a.offset(5,5,0x6667); a.add(4,4,0); a.move(22,3); a.offset(0,4,0x2);
        a.product(0,0,6); a.multiply_high(0,5,0); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3);
        a.store(1,0x68,0); a.jump(0x102794c0);
    }
    a.at(0x102794c0);
    primary_phase_54(a,state);
}
void primary_phase_52(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102793d4); a.sign8(0,3);
    primary_phase_53(a,state);
}
void primary_phase_51(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279388); a.sign8(0,3); a.jump(0x102793d8);
    primary_phase_53(a,state);
}
void primary_phase_50(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279324); a.load<1>(0,15,0x27); a.sign8(0,0); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10279390); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x102793a4); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x102793b4); a.load<1>(0,15,0x106); a.sign8(0,0,true);
                if (a.less()) {
                } else {
                    a.at(0x102793c0); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,1);
                    if (a.positive()) {
                    } else {
                        a.at(0x102793d0); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x102793d4);
        primary_phase_52(a,state);
    } else {
        a.at(0x10279340); a.load<1>(0,15,0x105); a.value(3,0x0); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
            a.at(0x10279354); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.positive()) {
            } else {
                a.at(0x10279364); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                } else {
                    a.at(0x10279374); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,11);
                    if (a.positive()) {
                    } else {
                        a.at(0x10279384); a.value(3,0x1);
                    }
                }
            }
        }
        a.at(0x10279388);
        primary_phase_51(a,state);
    }
}
void primary_phase_49(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102792e4); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102792ec); a.load<1>(4,15,0x27); a.value(0,0xb); a.load(3,15,0x1a4); a.value(6,0x4); a.sign8(5,4);
        a.load<1>(4,15,0x105); a.leading_zeroes(7,5); a.load<1>(5,15,0x106); a.mask(7,7,0x1,false,27);
        a.negate(7,7); a.intersect(0,0,7); a.sign8(7,0); a.call(0x101ef3c0); a.sign8(17,3);
    }
    a.at(0x10279324);
    primary_phase_50(a,state);
}
void primary_phase_48(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102792e0); a.value(0,0x0);
    primary_phase_49(a,state);
}
void primary_phase_47(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102792d8); a.value(0,0x1); a.jump(0x102792e4);
    primary_phase_49(a,state);
}
void primary_phase_46(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102792bc); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x102792e0);
        primary_phase_48(a,state);
    } else {
        a.at(0x102792c8); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x102792e0);
            primary_phase_48(a,state);
        } else {
            a.at(0x102792d8);
            primary_phase_47(a,state);
        }
    }
}
void primary_phase_45(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102792a0); a.load<1>(0,15,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x102792bc);
        primary_phase_46(a,state);
    } else {
        a.at(0x102792ac); a.load<1>(0,15,0x106); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x102792d8);
            primary_phase_47(a,state);
        } else {
            a.at(0x102792bc);
            primary_phase_46(a,state);
        }
    }
}
void primary_phase_44(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279260); a.exclusive_flags(3,14,0x80000000); a.value(0,0x43300000); a.store(1,0xe8,3);
    a.floating_load(1,26,128); a.store(1,0xe4,0); a.floating_load(2,26,48); a.floating_load(0,1,228);
    a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.floating_integer(0,0); a.floating_store(0,1,236);
    a.load(0,1,0xf0); a.store<2>(15,0x2a,0); a.jump(0x102792a0);
    primary_phase_45(a,state);
}
void primary_phase_43(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279254); a.compare(14,32000);
    if (a.positive()) {
        a.at(0x1027925c); a.value(14,0x7d00);
    }
    a.at(0x10279260);
    primary_phase_44(a,state);
}
void primary_phase_42(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279244); a.offset(14,13,-0x1f4); a.compare(14,-2000);
    if (a.less()) {
        a.at(0x10279250); a.value(14,0xfffff830);
    }
    a.at(0x10279254);
    primary_phase_43(a,state);
}
void primary_phase_41(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279240); a.move(13,0);
    primary_phase_42(a,state);
}
void primary_phase_40(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102790a4); a.value(4,0x55550000); a.value(0,0x43300000); a.offset(3,14,0x1f4); a.load(9,30,0x0);
    a.offset(4,4,0x5556); a.value(8,0x80010000); a.multiply_high(6,4,3); a.store(1,0xe4,0); a.value(5,0x43300000);
    a.floating_load(2,26,128); a.value(7,0x10000); a.floating_load(0,26,0); a.mask(0,6,0x1,false,1);
    a.value(3,0x55550000); a.add(0,6,0); a.value(4,0x80010000); a.exclusive_flags(0,0,0x80000000);
    a.load<2>(9,9,0x0); a.store(1,0xe8,0); a.offset(8,8,-0x7fff); a.load(6,27,0x0); a.offset(7,7,-0x1);
    a.floating_load(1,1,228); a.offset(0,14,0x1f4); a.add(9,9,6); a.offset(3,3,0x5556); a.floating_subtract(2,1,2);
    a.multiply_high(3,3,0); a.store(1,0xec,5); a.offset(4,4,-0x7fff); a.floating_load(1,26,128);
    a.floating_difference(2,2,0,true); a.multiply_high(6,8,9); a.mask(0,3,0x1,false,1); a.add(0,3,0); a.add(5,6,9);
    a.shift_right(5,5,0xf); a.mask(6,5,0x1,false,1); a.add(5,5,6); a.product(5,5,7); a.subtract(13,5,9);
    a.exclusive_flags(5,13,0x80000000); a.store(1,0xf0,5); a.multiply(0,0,0xa); a.floating_load(0,1,236);
    a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
    a.floating_store(0,1,244); a.load(13,1,0xf8); a.call(0x10226520); a.value(5,0x66660000); a.value(4,0x55550000);
    a.offset(3,3,0x5); a.offset(0,14,0x1f4); a.offset(5,5,0x6667); a.offset(4,4,0x5556); a.multiply_high(5,5,3);
    a.multiply_high(3,4,0); a.shift_right(4,5,0x2); a.mask(5,4,0x1,false,1); a.mask(0,3,0x1,false,1); a.add(4,4,5);
    a.add(3,3,0); a.add(13,13,4); a.offset(0,3,-0x1); a.compare_registers(13,0);
    if (a.positive()) {
        a.at(0x102791c8); a.value(3,0x55550000); a.offset(0,14,0x1f4); a.offset(3,3,0x5556);
        a.multiply_high(3,3,0); a.mask(0,3,0x1,false,1); a.add(3,3,0); a.offset(13,3,-0x1); a.jump(0x10279244);
    }
    a.at(0x10279244);
    primary_phase_42(a,state);
}
void primary_phase_39(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10279054); a.value(4,0x55550000); a.value(3,0x10000); a.offset(0,14,0x1f4); a.offset(4,4,0x5556);
    a.offset(3,3,-0x1); a.multiply_high(4,4,0); a.mask(0,4,0x1,false,1); a.add(0,4,0); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x102790a4);
        primary_phase_40(a,state);
    } else {
        a.at(0x1027907c); a.value(4,0x55550000); a.value(3,0xffff0000); a.offset(0,14,0x1f4); a.offset(4,4,0x5556);
        a.offset(3,3,0x1); a.multiply_high(4,4,0); a.mask(0,4,0x1,false,1); a.add(0,4,0); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x102791e8); a.value(4,0x55550000); a.load(5,30,0x0); a.offset(0,14,0x1f4); a.value(3,0x55550000);
            a.offset(4,4,0x5556); a.load<2>(6,5,0x0); a.multiply_high(5,4,0); a.load(4,27,0x0);
            a.offset(0,14,0x1f4); a.add(6,6,4); a.offset(3,3,0x5556); a.mask(4,5,0x1,false,1); a.add(5,5,4);
            a.divide(4,6,5); a.multiply_high(3,3,0); a.product(4,4,5); a.mask(0,3,0x1,false,1);
            a.add_record(0,3,0); a.subtract(13,4,6); a.negate(0,13);
            if (a.positive()) {
                a.at(0x1027923c); a.move(0,13);
            }
            a.at(0x10279240);
            primary_phase_41(a,state);
        } else {
            a.at(0x102790a4);
            primary_phase_40(a,state);
        }
    }
}
void primary_phase_38(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278fb4); a.load<1>(0,1,0xfd); a.load<2>(3,1,0x58,true); a.sign8(4,0); a.value(0,0x43300000);
    a.product(3,4,3); a.load<2>(5,1,0x58,true); a.store(1,0xe4,0); a.floating_load(1,26,128);
    a.floating_load(2,26,48); a.product(0,5,3); a.exclusive_flags(0,0,0x80000000); a.store(1,0xe8,0);
    a.floating_load(0,1,228); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
    a.floating_move(0,0); state.wr<8>(16384,state.r<8>(16384)^UINT64_C(0x8000000000000000));
    a.floating_integer(0,0); a.floating_store(0,1,236); a.load(14,1,0xf0); a.compare(14,-500);
    if (a.at_least()) {
        a.at(0x10279254);
        primary_phase_43(a,state);
    } else {
        a.at(0x10279008); a.value(3,0x55550000); a.offset(0,14,0x1f4); a.offset(3,3,0x5556);
        a.multiply_high(3,3,0); a.mask(0,3,0x1,false,1); a.add_record(0,3,0);
        if (!a.equal()) {
            a.at(0x1027902c); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10279044); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(27,0x0,0);
            }
            a.at(0x10279054);
            primary_phase_39(a,state);
        } else {
            a.at(0x10279024); a.value(13,0x0); a.jump(0x10279244);
            primary_phase_42(a,state);
        }
    }
}
void primary_phase_37(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278f80); a.floating_load(1,31,232,true); a.floating_load(0,31,324,true);
    a.floating_product_sum(0,1,29,0,true); a.floating_integer(0,0); a.floating_store(0,1,228); a.load(0,1,0xe8);
    a.store<2>(15,0x2a,0); a.jump(0x102792a0);
    primary_phase_45(a,state);
}
void primary_phase_36(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278e48); a.load(3,15,0x1a4); a.value(6,0x0); a.load<1>(4,15,0x106); a.load<1>(5,15,0x27);
    a.call(0x101ee680); a.move(0,3); a.load(3,15,0x1a4); a.store<1>(1,0x100,0); a.move(4,15); a.value(17,0x0);
    a.call(0x101ee770); a.load<1>(0,15,0x27); a.move(4,3); a.load(3,15,0x1a4); a.value(7,0x1); a.sign8(0,0);
    a.store<1>(1,0xff,4); a.leading_zeroes(0,0); a.load<1>(5,15,0x105); a.mask(0,0,0x7ffffff,false,27);
    a.load<1>(6,15,0x106); a.sign8(4,0); a.call(0x101e3f10); a.load<1>(0,15,0x27); a.floating_move(29,1);
    a.load(3,15,0x1a4); a.value(7,0x1); a.sign8(0,0); a.load<1>(5,15,0x105); a.leading_zeroes(0,0);
    a.mask(0,0,0x7ffffff,false,27); a.load<1>(6,15,0x106); a.sign8(4,0); a.call(0x101e4020); a.load<1>(0,15,0x27);
    a.move(4,3); a.store<1>(1,0xfe,4); a.sign8(4,0); a.value(0,0x0); a.store<1>(1,0xfd,0); a.value(0,0x0);
    a.leading_zeroes(4,4); a.load(3,15,0x1a4); a.store<1>(1,0xfc,0); a.mask(0,4,0x7ffffff,false,27); a.sign8(4,0);
    a.call(0x101f1e10); a.move(0,3); a.load(3,15,0x1a4); a.load<1>(4,15,0x27); a.move(23,0); a.offset(5,1,0x4b);
    a.offset(6,1,0x4a); a.offset(7,1,0x49); a.offset(8,1,0x48); a.offset(9,1,0x47); a.value(10,0x0);
    a.call(0x101f21d0); a.sign8(0,16); a.compare(0,4);
    if (a.equal()) {
        a.at(0x102794b8); a.value(0,0x15e); a.store<2>(15,0x2a,0);
        primary_phase_54(a,state);
    } else {
        a.at(0x10278f3c); a.load(3,15,0x1a4); a.load<1>(0,3,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x10279298); a.value(0,0xc8); a.store<2>(15,0x2a,0);
            primary_phase_45(a,state);
        } else {
            a.at(0x10278f50); a.load(3,15,0x1a4); a.move(4,15); a.offset(5,1,0x58); a.offset(6,1,0x74);
            a.call(0x101d7dc0); a.sign8(0,3,true); a.store<1>(1,0xfd,0);
            if (!a.equal()) {
                a.at(0x10278fa0); a.load<2>(0,1,0x58,true); a.compare(0,0);
                if (a.less()) {
                    a.at(0x10278fac); a.value(0,0x0); a.store<2>(1,0x58,0);
                }
                a.at(0x10278fb4);
                primary_phase_38(a,state);
            } else {
                a.at(0x10278f70); a.floating_load(0,26,264); a.floating_compare(29,0);
                if (a.positive()) {
                    a.at(0x10278f7c); a.floating_load(29,31,320,true);
                }
                a.at(0x10278f80);
                primary_phase_37(a,state);
            }
        }
    }
}
void primary_phase_35(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278dd4); a.load<1>(0,15,0x27); a.compare(0,1);
    if (a.equal()) {
        a.at(0x10278de0); a.load<1>(3,15,0x106); a.load<1>(0,1,0x52); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.at_least()) {
        } else {
            a.at(0x10278df8); a.load<1>(3,1,0x53); a.load<1>(0,15,0x105); a.sign8(3,3); a.load<1>(13,1,0x52);
            a.sign8(0,0); a.subtract(3,3,0); a.call(0x10775540); a.load<1>(0,15,0x106); a.sign8(4,13);
            a.sign8(13,3); a.sign8(0,0); a.subtract(3,4,0); a.call(0x10775540); a.sign8(3,3); a.sign8(0,13);
            a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.load(3,2,-0x6424);
            a.floating_indexed_single(30,3,0);
        }
    }
    a.at(0x10278e48);
    primary_phase_36(a,state);
}
void primary_phase_34(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278cfc); a.sign8(0,16); a.compare(0,4);
    if (!a.equal()) {
        a.at(0x10278d34); a.load(3,15,0x1a4); a.move(4,15); a.offset(5,1,0x53); a.offset(6,1,0x52); a.value(7,0x1);
        a.call(0x101e28b0); a.load<1>(4,1,0x53); a.move(3,15); a.call(0x102857c0); a.store<1>(1,0x53,3);
        a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10278dd4);
            primary_phase_35(a,state);
        } else {
            a.at(0x10278d68); a.load<1>(3,15,0x106); a.load<1>(0,1,0x52); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (a.nonpositive()) {
                a.at(0x10278dd4);
                primary_phase_35(a,state);
            } else {
                a.at(0x10278d80); a.load<1>(3,1,0x53); a.load<1>(0,15,0x105); a.sign8(3,3); a.load<1>(13,1,0x52);
                a.sign8(0,0); a.subtract(3,3,0); a.call(0x10775540); a.load<1>(0,15,0x106); a.sign8(4,13);
                a.sign8(13,3); a.sign8(0,0); a.subtract(3,4,0); a.call(0x10775540); a.sign8(3,3); a.sign8(0,13);
                a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.load(3,2,-0x6424);
                a.floating_indexed_single(30,3,0); a.jump(0x10278e48);
                primary_phase_36(a,state);
            }
        }
    } else {
        a.at(0x10278d08); a.value(0,0x4); a.store<1>(1,0x53,0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10278d28); a.value(0,0x0); a.store<1>(1,0x52,0); a.jump(0x10278e48);
        } else {
            a.at(0x10278d1c); a.value(0,0xb); a.store<1>(1,0x52,0); a.jump(0x10278e48);
        }
        a.at(0x10278e48);
        primary_phase_36(a,state);
    }
}
void primary_phase_33(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278ce4); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13);
    a.jump(0x1028175c);
}
void primary_phase_32(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278cc0); a.value(0,0x37); a.move(3,15); a.store<2>(15,0x19c,0); a.move(5,22); a.value(4,0x37);
    a.load<1>(6,1,0x56); a.load<1>(7,1,0x55); a.load<1>(8,1,0x54); a.call(0x10245220);
    primary_phase_33(a,state);
}
void primary_phase_31(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278cbc); a.value(22,0x0);
    primary_phase_32(a,state);
}
void primary_phase_30(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278c84); a.value(0,0x4); a.store<1>(1,0x55,0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10278cac); a.load(3,15,0x1a4); a.load<1>(3,3,0x7926); a.offset(0,3,0x2); a.store<1>(1,0x54,0);
    } else {
        a.at(0x10278c98); a.load(3,15,0x1a4); a.load<1>(3,3,0x7925); a.offset(0,3,-0x2); a.store<1>(1,0x54,0);
        a.jump(0x10278cbc);
    }
    a.at(0x10278cbc);
    primary_phase_31(a,state);
}
void primary_phase_29(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278c3c); a.load(4,15,0x1a4); a.value(0,0x37); a.load(3,15,0x1a4); a.offset(6,1,0x41);
    a.load<1>(5,4,0x791e); a.offset(7,1,0x56); a.load<1>(4,3,0x791d); a.offset(8,1,0x55); a.offset(9,1,0x54);
    a.offset(10,1,0x68); a.store(1,0x38,0); a.load(3,15,0x1a4); a.call(0x101e8950); a.move(22,3,true);
    if (a.equal()) {
        a.at(0x10278c84);
        primary_phase_30(a,state);
    } else {
        a.at(0x10278c78); a.load(0,1,0x68); a.compare(0,2000);
        if (a.at_least()) {
            a.at(0x10278cc0);
            primary_phase_32(a,state);
        } else {
            a.at(0x10278c84);
            primary_phase_30(a,state);
        }
    }
}
void primary_phase_28(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278bc0); a.load(4,30,0x0); a.value(3,0x51ec0000); a.load(0,27,0x0); a.offset(3,3,-0x7ae1);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x64); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x10278c18); a.load<2>(6,1,0x5c,true); a.move(3,15); a.sign16(5,21); a.value(4,0x35); a.value(7,0x1);
        a.value(8,0x0); a.value(9,0x0); a.call(0x102576e0); a.jump(0x10278ce4);
    } else {
        a.at(0x10278bf4); a.load<2>(6,1,0x5c,true); a.move(3,15); a.sign16(5,21); a.value(4,0x35); a.value(7,0x6);
        a.value(8,0x0); a.value(9,0x0); a.call(0x102576e0); a.jump(0x10278ce4);
    }
    a.at(0x10278ce4);
    primary_phase_33(a,state);
}
void primary_phase_27(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278b30); a.sign8(0,13); a.value(4,0x55550000); a.add(3,14,0); a.floating_load(1,26,128);
    a.offset(4,4,0x5556); a.value(0,0x43300000); a.multiply_high(3,4,3); a.store(1,0xe4,0);
    a.mask(0,3,0x1,false,1); a.add(0,3,0); a.exclusive_flags(0,0,0x80000000); a.store(1,0xe8,0);
    a.floating_load(0,1,228); a.floating_subtract(0,0,1,true); a.floating_compare(31,0);
    if (a.at_least()) {
        a.at(0x10278c3c);
        primary_phase_29(a,state);
    } else {
        a.at(0x10278b70); a.load(5,15,0x1a4); a.value(4,0x6); a.value(0,0x35); a.move(3,15);
        a.store<1>(5,0x7927,4); a.offset(4,1,0x40); a.offset(5,1,0x5c); a.store<1>(1,0x40,0); a.call(0x10252080);
        a.load(4,30,0x0); a.sign16(21,3); a.load(0,29,0x0); a.offset(3,4,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10278bb0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10278bc0);
        primary_phase_28(a,state);
    }
}
void primary_phase_26(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278b2c); a.move(14,0);
    primary_phase_27(a,state);
}
void primary_phase_25(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278a1c); a.exclusive_flags(3,16,0x80000000); a.value(0,0x43300000); a.store(1,0xf8,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0xf4,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,244); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,27,0x0);
    a.multiply(0,16,0xa); a.offset(3,3,-0x7fff); a.store(1,0xec,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(14,4,7);
    a.exclusive_flags(4,14,0x80000000); a.store(1,0xf0,4); a.multiply_high(3,3,0); a.floating_load(0,1,236);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,228); a.load(14,1,0xe8);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,16,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(14,14,3);
    a.compare_registers(14,0);
    if (a.positive()) {
        a.at(0x10278af8); a.offset(14,16,-0x1); a.jump(0x10278b30);
    }
    a.at(0x10278b30);
    primary_phase_27(a,state);
}
void primary_phase_24(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102789fc); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(16,3);
    if (a.positive()) {
        a.at(0x10278a1c);
        primary_phase_25(a,state);
    } else {
        a.at(0x10278a0c); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(16,3);
        if (a.at_least()) {
            a.at(0x10278b00); a.load(3,30,0x0); a.compare(16,0); a.load(0,27,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,16); a.product(0,0,16); a.subtract(14,0,3); a.negate(0,14);
            if (a.positive()) {
                a.at(0x10278b28); a.move(0,14);
            }
            a.at(0x10278b2c);
            primary_phase_26(a,state);
        } else {
            a.at(0x10278a1c);
            primary_phase_25(a,state);
        }
    }
}
void primary_phase_23(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278994); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10278c3c);
        primary_phase_29(a,state);
    } else {
        a.at(0x1027899c); a.floating_load(0,26,896); a.floating_compare(31,0);
        if (a.at_least()) {
            a.at(0x10278c3c);
            primary_phase_29(a,state);
        } else {
            a.at(0x102789a8); a.floating_load(1,31,240,true); a.floating_load(0,15,164,true);
            a.load<1>(13,15,0x10c); a.floating_sum_single(0,1,0); a.floating_integer(0,0);
            a.floating_store(0,1,244); a.load(16,1,0xf8); a.compare(16,0);
            if (!a.equal()) {
                a.at(0x102789d4); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x102789ec); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x102789fc);
                primary_phase_24(a,state);
            } else {
                a.at(0x102789cc); a.value(14,0x0); a.jump(0x10278b30);
                primary_phase_27(a,state);
            }
        }
    }
}
void primary_phase_22(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278988); a.value(0,0x1); a.jump(0x10278994);
    primary_phase_23(a,state);
}
void primary_phase_21(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278920); a.value(0,0x37); a.move(3,15); a.store<2>(15,0x19c,0); a.move(5,22); a.value(4,0x37);
    a.load<1>(6,1,0x56); a.load<1>(7,1,0x55); a.load<1>(8,1,0x54); a.call(0x10245220); a.offset(3,1,0x74);
    a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_20(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027891c); a.value(22,0x0);
    primary_phase_21(a,state);
}
void primary_phase_19(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102788e4); a.value(0,0x4); a.store<1>(1,0x55,0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1027890c); a.load(3,15,0x1a4); a.load<1>(3,3,0x7926); a.offset(0,3,0x2); a.store<1>(1,0x54,0);
    } else {
        a.at(0x102788f8); a.load(3,15,0x1a4); a.load<1>(3,3,0x7925); a.offset(0,3,-0x2); a.store<1>(1,0x54,0);
        a.jump(0x1027891c);
    }
    a.at(0x1027891c);
    primary_phase_20(a,state);
}
void primary_phase_18(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278878); a.offset(3,1,0x74); a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13);
    a.jump(0x1028175c);
}
void primary_phase_17(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278800); a.load(4,30,0x0); a.value(3,0x51ec0000); a.load(0,27,0x0); a.offset(3,3,-0x7ae1);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x64); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x10278858); a.load<2>(6,1,0x5c,true); a.move(3,15); a.sign16(5,21); a.value(4,0x33); a.value(7,0x1);
        a.value(8,0x0); a.value(9,0x0); a.call(0x102576e0);
    } else {
        a.at(0x10278834); a.load<2>(6,1,0x5c,true); a.move(3,15); a.sign16(5,21); a.value(4,0x33); a.value(7,0x5);
        a.value(8,0x0); a.value(9,0x0); a.call(0x102576e0); a.jump(0x10278878);
    }
    a.at(0x10278878);
    primary_phase_18(a,state);
}
void primary_phase_16(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278778); a.value(0,0x5a); a.move(3,15); a.store<2>(15,0x19c,0); a.value(4,0x5a); a.load<1>(6,1,0x56);
    a.load<1>(7,1,0x55); a.load<1>(8,1,0x54); a.call(0x10245220); a.offset(3,1,0x74); a.value(13,0x0);
    a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_15(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278774); a.value(5,0x0);
    primary_phase_16(a,state);
}
void primary_phase_14(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278740); a.load(4,30,0x0); a.value(3,0x55550000); a.load(0,27,0x0); a.offset(3,3,0x5556);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(3,3,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4); a.subtract_from(0,0,0x5); a.store<1>(1,0x54,0);
    primary_phase_15(a,state);
}
void primary_phase_13(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102786e0); a.load(4,30,0x0); a.value(3,0x55550000); a.load(0,27,0x0); a.offset(3,3,0x5556);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(3,3,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(3,0,4); a.offset(0,3,0x6); a.store<1>(1,0x54,0); a.jump(0x10278774);
    primary_phase_15(a,state);
}
void primary_phase_12(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278670); a.load(4,30,0x0); a.value(3,0x92490000); a.load(0,27,0x0); a.offset(3,3,0x2493);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.add(0,0,4); a.shift_right(0,0,0x2);
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0x7); a.subtract(0,0,4); a.subtract_from(0,0,0x7);
    a.store<1>(1,0x55,0); a.load<1>(0,15,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10278718); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10278730); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x10278740);
        primary_phase_14(a,state);
    } else {
        a.at(0x102786b8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x102786d0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x102786e0);
        primary_phase_13(a,state);
    }
}
void primary_phase_11(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278648); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x10278660); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(27,0x0,0);
    }
    a.at(0x10278670);
    primary_phase_12(a,state);
}
void primary_phase_10(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102785b8); a.value(0,0x36); a.move(3,15); a.store<2>(15,0x19c,0); a.move(5,13); a.value(4,0x36);
    a.load<1>(6,1,0x56); a.load<1>(7,1,0x55); a.load<1>(8,1,0x54); a.call(0x10245220); a.offset(3,1,0x74);
    a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_9(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102785b4); a.store<1>(1,0x55,0);
    primary_phase_10(a,state);
}
void primary_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027858c); a.load(3,30,0x0); a.load(0,27,0x0); a.load<2>(3,3,0x0); a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30); a.mask(3,3,0x1,false,1); a.subtract(0,3,0); a.mask(0,0,0xffffffff,false,2);
    a.add(0,0,3); a.subtract_from(0,0,0x8);
    primary_phase_9(a,state);
}
void primary_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027853c); a.load(3,30,0x0); a.load(0,27,0x0); a.load<2>(3,3,0x0); a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30); a.mask(3,3,0x1,false,1); a.subtract(0,3,0); a.mask(0,0,0xffffffff,false,2);
    a.add(0,0,3); a.jump(0x102785b4);
    primary_phase_9(a,state);
}
void primary_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278504); a.load(3,15,0x1a4); a.load<1>(0,3,0x791d); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10278564); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027857c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027858c);
        primary_phase_8(a,state);
    } else {
        a.at(0x10278514); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027852c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(27,0x0,0);
        }
        a.at(0x1027853c);
        primary_phase_7(a,state);
    }
}
void primary_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102784ec); a.load<1>(0,1,0x54); a.sign8(0,0); a.compare(0,11);
    if (a.positive()) {
        a.at(0x102784fc); a.value(0,0xb); a.store<1>(1,0x54,0);
    }
    a.at(0x10278504);
    primary_phase_6(a,state);
}
void primary_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102784d0); a.add(0,3,4); a.store<1>(1,0x54,0); a.load<1>(0,1,0x54); a.sign8(0,0,true);
    if (a.less()) {
        a.at(0x102784e4); a.value(0,0x0); a.store<1>(1,0x54,0);
    }
    a.at(0x102784ec);
    primary_phase_5(a,state);
}
void primary_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10278430); a.value(0,0x34); a.move(3,15); a.store<2>(15,0x19c,0); a.move(5,22); a.value(4,0x34);
    a.load<1>(6,1,0x56); a.load<1>(7,1,0x55); a.load<1>(8,1,0x54); a.call(0x10245220); a.offset(3,1,0x74);
    a.value(13,0x0); a.value(4,0xffffffff); a.call(0x10738c20); a.move(3,13); a.jump(0x1028175c);
}
void primary_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102783d0); a.load(4,30,0x0); a.value(3,0x55550000); a.load(0,27,0x0); a.offset(3,3,0x5556);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(3,3,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4); a.subtract_from(0,0,0x5); a.store<1>(1,0x55,0); a.load<1>(0,15,0x27);
    a.compare(0,1);
    if (!a.equal()) {
        a.at(0x1027841c); a.load<1>(0,15,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
        } else {
            a.at(0x10278428); a.value(0,0xb); a.store<1>(1,0x54,0);
        }
    } else {
        a.at(0x10278410); a.value(0,0x0); a.store<1>(1,0x54,0); a.jump(0x10278430);
    }
    a.at(0x10278430);
    primary_phase_3(a,state);
}
void primary_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102782f0); a.value(0,0x0); a.value(5,0x0); a.store<2>(1,0x58,0); a.value(0,0x0); a.value(4,0xffffffff);
    a.floating_move(31,0); a.store<1>(1,0x102,0); a.value(0,0x0); a.floating_load(30,31,0,true);
    a.offset(3,1,0x74); a.store<1>(1,0x56,5); a.store<1>(1,0x101,0); a.store<1>(1,0x51,4); a.call(0x10738c00);
    a.load(3,15,0x1a4); a.load<1>(0,3,0x7928); a.sign8(0,0); a.compare(0,8,false);
    if (a.positive()) {
        a.at(0x10278cfc);
        primary_phase_34(a,state);
    } else {
        a.at(0x1027833c); a.load(3,2,-0x525c); a.mask(0,0,0xfffffffc,false,2); a.indexed_load(3,3,0);
        a.count_destination(3); a.count_branch();
        switch (a.r(0) >> 2) {
            case 0: case 1: {
                a.at(0x10278cfc);
                primary_phase_34(a,state);
                break;
            }
            case 2: {
                a.at(0x1027846c); a.load(4,15,0x1a4); a.value(3,0x0); a.value(0,0x36); a.offset(6,1,0x45);
                a.store<1>(4,0x7928,3); a.offset(7,1,0x56); a.offset(8,1,0x55); a.offset(9,1,0x54);
                a.load(4,15,0x1a4); a.offset(10,1,0x68); a.load(3,15,0x1a4); a.load<1>(5,4,0x791e);
                a.load<1>(4,3,0x791d); a.store(1,0x38,0); a.load(3,15,0x1a4); a.call(0x101e8950);
                a.move(13,3,true);
                if (!a.equal()) {
                    a.at(0x102785b8);
                    primary_phase_10(a,state);
                } else {
                    a.at(0x102784b4); a.load<1>(0,15,0x27); a.value(3,0x1); a.load(4,15,0x1a4); a.compare(0,1);
                    a.load<1>(4,4,0x791e);
                    if (a.equal()) {
                        a.at(0x102784cc); a.value(3,0xffffffff);
                    }
                    a.at(0x102784d0);
                    primary_phase_4(a,state);
                }
                break;
            }
            case 3: {
                a.at(0x10278360); a.load(4,15,0x1a4); a.value(3,0x0); a.value(0,0x34); a.offset(6,1,0x46);
                a.store<1>(4,0x7928,3); a.offset(7,1,0x56); a.offset(8,1,0x55); a.offset(9,1,0x54);
                a.load(4,15,0x1a4); a.offset(10,1,0x68); a.load(3,15,0x1a4); a.load<1>(5,4,0x791e);
                a.load<1>(4,3,0x791d); a.store(1,0x38,0); a.load(3,15,0x1a4); a.call(0x101e8950);
                a.move(22,3,true);
                if (!a.equal()) {
                    a.at(0x10278430);
                    primary_phase_3(a,state);
                } else {
                    a.at(0x102783a8); a.load(3,30,0x0); a.load(0,29,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x102783c0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(27,0x0,0);
                    }
                    a.at(0x102783d0);
                    primary_phase_2(a,state);
                }
                break;
            }
            case 4: {
                a.at(0x102785f4); a.load(4,15,0x1a4); a.value(3,0x0); a.value(0,0x5a); a.offset(6,1,0x44);
                a.store<1>(4,0x7928,3); a.offset(7,1,0x56); a.offset(8,1,0x55); a.offset(9,1,0x54);
                a.load(4,15,0x1a4); a.offset(10,1,0x68); a.load(3,15,0x1a4); a.load<1>(5,4,0x791e);
                a.load<1>(4,3,0x791d); a.store(1,0x38,0); a.load(3,15,0x1a4); a.call(0x101e8950); a.move(5,3,true);
                if (a.equal()) {
                    a.at(0x10278648);
                    primary_phase_11(a,state);
                } else {
                    a.at(0x1027863c); a.load(0,1,0x68); a.compare(0,2000);
                    if (a.at_least()) {
                        a.at(0x10278778);
                        primary_phase_16(a,state);
                    } else {
                        a.at(0x10278648);
                        primary_phase_11(a,state);
                    }
                }
                break;
            }
            case 5: {
                a.at(0x102787b0); a.load(5,15,0x1a4); a.value(4,0x0); a.value(0,0x33); a.move(3,15);
                a.store<1>(5,0x7928,4); a.offset(4,1,0x43); a.offset(5,1,0x5c); a.store<1>(1,0x43,0);
                a.call(0x10252080); a.load(4,30,0x0); a.sign16(21,3); a.load(0,29,0x0); a.offset(3,4,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x102787f0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(27,0x0,0);
                }
                a.at(0x10278800);
                primary_phase_17(a,state);
                break;
            }
            case 6: {
                a.at(0x1027895c); a.load(3,15,0x1a4); a.value(0,0x0); a.store<1>(3,0x7928,0);
                a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,2);
                if (a.less()) {
                    a.at(0x10278988);
                    primary_phase_22(a,state);
                } else {
                    a.at(0x10278978); a.load<1>(0,15,0x105); a.sign8(0,0); a.compare(0,6);
                    if (a.nonpositive()) {
                        a.at(0x10278990); a.value(0,0x0);
                        primary_phase_23(a,state);
                    } else {
                        a.at(0x10278988);
                        primary_phase_22(a,state);
                    }
                }
                break;
            }
            case 7: {
                a.at(0x10278890); a.load(4,15,0x1a4); a.value(3,0x0); a.value(0,0x37); a.offset(6,1,0x42);
                a.store<1>(4,0x7928,3); a.offset(7,1,0x56); a.offset(8,1,0x55); a.offset(9,1,0x54);
                a.load(4,15,0x1a4); a.offset(10,1,0x68); a.load(3,15,0x1a4); a.load<1>(5,4,0x791e);
                a.load<1>(4,3,0x791d); a.store(1,0x38,0); a.load(3,15,0x1a4); a.call(0x101e8950);
                a.move(22,3,true);
                if (a.equal()) {
                    a.at(0x102788e4);
                    primary_phase_19(a,state);
                } else {
                    a.at(0x102788d8); a.load(0,1,0x68); a.compare(0,2000);
                    if (a.at_least()) {
                        a.at(0x10278920);
                        primary_phase_21(a,state);
                    } else {
                        a.at(0x102788e4);
                        primary_phase_19(a,state);
                    }
                }
                break;
            }
            case 8: {
                a.at(0x10278350); a.load(3,15,0x1a4); a.value(0,0x0); a.store<1>(3,0x7928,0); a.jump(0x10278cfc);
                primary_phase_34(a,state);
                break;
            }
        }
    }
}
}
void select_primary_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10278220,dependencies,observer);
    a.at(0x10278220); a.link(); a.move(11,1); a.call(0x10773808); a.multiple(13,false,-32); a.save_link();
    a.frame(384); a.load<1>(0,3,0x27); a.move(15,3); a.load(26,2,-0x51e8); a.move(16,4); a.sign8(0,0,true);
    a.load(27,2,-0x7b90); a.load(28,2,-0x7b8c); a.value(20,0x0); a.load(29,2,-0x7b88); a.value(19,0x0);
    a.load(30,2,-0x7b84); a.load(31,2,-0x51e0);
    if (!a.equal()) {
        a.at(0x102782b0); a.load<1>(0,15,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540); a.move(0,3);
        a.load<1>(3,15,0x106); a.sign8(13,0); a.sign8(3,3); a.call(0x10775540); a.sign8(3,3); a.sign8(0,13);
        a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.load(3,2,-0x6424);
        a.floating_indexed_single(0,3,0);
    } else {
        a.at(0x1027826c); a.load<1>(0,15,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540);
        a.load<1>(0,15,0x106); a.sign8(13,3); a.sign8(3,0); a.offset(3,3,-0xb); a.call(0x10775540); a.sign8(3,3);
        a.sign8(0,13); a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.load(3,2,-0x6424);
        a.floating_indexed_single(0,3,0); a.jump(0x102782f0);
    }
    a.at(0x102782f0);
    primary_phase_1(a,state);
    a.at(0x1028175c); a.load(0,1,0x188); a.offset(1,1,0x180); a.move(11,1); a.call(0x10773858); a.restore_link();
    a.multiple(13,true,-32); a.finish();
}
}
