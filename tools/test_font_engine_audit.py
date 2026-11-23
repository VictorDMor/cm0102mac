"""Certify the font engine's 263 entries (T2K and its autogridder: tools/font_engine_audit.py), the unit bounds, the API
boundary, the function-pointer entries and which entries are routed to C++ (the blocks util and stream).

The private export is required evidence, rather than a reason to skip tests. Production ownership is asserted, not skipped:
exactly the routed entries are listed, owned and hooked, and the hook checker rejects a page that carries a hook for an entry
that is not hooked or lacks one for an entry that is.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import struct
import tempfile
import unittest
from unittest import mock

import font_engine_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
TOC = 0x108bba90
CODE_END = 0x10781330
SPAN = audit.MODULE
CONTRACTS = (
    (0x10751350, 0x10751b0c, 495, '7e52bf3e64006ba5'), (0x10751b10, 0x10751bd0, 48, '9e13dde6efea098f'),
    (0x10751bd0, 0x10751eb0, 184, '5c855c5a227bea67'), (0x10751eb0, 0x10751fc8, 70, '581fc1ea40f45f72'),
    (0x10751fd0, 0x107522b0, 184, 'c5cda22c6b59706e'), (0x107522b0, 0x107524a4, 125, '70e03e510805f8b4'),
    (0x107524b0, 0x10752984, 309, 'd498fb21dd0ceb34'), (0x10752990, 0x10753104, 477, 'e50d97adce586645'),
    (0x10753110, 0x10753560, 275, '8962071c898a0f1d'), (0x10753560, 0x107536b0, 84, '5bfad6a150cb110f'),
    (0x107536b0, 0x107537bc, 67, '5166431100eca276'), (0x107537c0, 0x1075397c, 111, '7da56be57c29bfac'),
    (0x10753980, 0x10753a80, 64, '4e7d81e0481d0ca2'), (0x10753a80, 0x10753aa4, 9, '09614dee42aeb615'),
    (0x10753ab0, 0x10753bec, 79, '5ab2800a490dc818'), (0x10753bf0, 0x10753c2c, 15, '99d2aef371ab69ab'),
    (0x10753c30, 0x10753c6c, 15, '964a4297c23a23bf'), (0x10753c70, 0x10753ccc, 23, '195f0711e9bd07f1'),
    (0x10753cd0, 0x10753d20, 20, 'b0dbde840db23d8c'), (0x10753d20, 0x10753d44, 9, '74107edd0626d62d'),
    (0x10753d50, 0x10753d54, 1, 'acde5b2612aabe8f'), (0x10753d60, 0x10753d64, 1, 'df09b2bedbd1e098'),
    (0x10753d70, 0x10753e70, 64, '16da85832ff81b5a'), (0x10753e70, 0x10753f70, 64, 'f7242f1a68963a05'),
    (0x10753f70, 0x10753f9c, 11, 'eb2ce7064a46b00a'), (0x10753fa0, 0x10753fcc, 11, 'dd29f189725da898'),
    (0x10753fd0, 0x1075404c, 31, '3489ce1ae321dcf0'), (0x10754050, 0x107540cc, 31, '9fd84bdd0a65a3dc'),
    (0x107540d0, 0x1075412c, 23, 'd4ba54ae49930b7b'), (0x10754130, 0x1075418c, 23, '2aa4b6e4519916fa'),
    (0x10754190, 0x1075436c, 117, 'bd2799a957446ee4'), (0x10754370, 0x1075454c, 117, 'a0be9a07af4c5b5f'),
    (0x10754550, 0x107545a4, 21, '23c2e3cf68ddf5ef'), (0x107545b0, 0x10754604, 21, '16b7f824109d9754'),
    (0x10754610, 0x10754804, 125, '98c08fc2c9f540ad'), (0x10754810, 0x107548cc, 47, '97e1bfb6fa063710'),
    (0x107548d0, 0x1075498c, 47, '07446cf1d867cd80'), (0x10754990, 0x10754cd0, 207, '7ff3c4c71f8dfe92'),
    (0x10754cd0, 0x10754f68, 166, 'f3b2583fdf3bd5ca'), (0x10754f70, 0x10754fec, 31, 'b182fff7cc515e2c'),
    (0x10754ff0, 0x107551b4, 113, '9db976a000b02d84'), (0x107551c0, 0x107552ac, 59, 'e9c854b9337d9181'),
    (0x107552b0, 0x107555cc, 199, '7f4e5df4d88f8193'), (0x107555d0, 0x10755838, 154, 'ee6297c4169a19ab'),
    (0x10755840, 0x10755924, 57, '1c04cd7426c12db6'), (0x10755930, 0x107559a0, 28, 'dc910cb2131ee3a1'),
    (0x107559a0, 0x107559c8, 10, 'd3cf094f9bb29ab9'), (0x107559d0, 0x10755a14, 17, '1e0c932d6993c7f5'),
    (0x10755a20, 0x10755a5c, 15, '2c149a05e0a3bf53'), (0x10755a60, 0x10755aa4, 17, '399cc6a3665c0304'),
    (0x10755ab0, 0x10755af4, 17, '4559ae642055474b'), (0x10755b00, 0x10755b4c, 19, 'e9ae656aaee0b5a1'),
    (0x10755b50, 0x10755b58, 2, '821e87b7ebe9a96b'), (0x10755b60, 0x10755be0, 32, '781000542eaa2828'),
    (0x10755be0, 0x10755cb8, 54, 'cf3f38e4e18579da'), (0x10755cc0, 0x10755dfc, 79, '6dab0a2ba8b7df5a'),
    (0x10755e00, 0x10755e28, 10, 'd81ea77bff74b44e'), (0x10755e30, 0x10755e58, 10, '7f73580b660608ce'),
    (0x10755e60, 0x10755eb4, 21, '64ad4cf63e899dbf'), (0x10755ec0, 0x10755f14, 21, 'beea6fb16bc37619'),
    (0x10755f20, 0x10755f28, 2, '3578ba5962a99b03'), (0x10755f30, 0x10755f38, 2, 'e2911ee24ee9b704'),
    (0x10755f40, 0x10755f54, 5, '0bc9cd1379edacf6'), (0x10755f60, 0x1075601c, 47, '807bb9bf2449d52f'),
    (0x10756020, 0x1075602c, 3, '91b31012843ab214'), (0x10756030, 0x107560d8, 42, '48ebfffe42083367'),
    (0x107560e0, 0x107560fc, 7, '197ae1d3ee3b3dbd'), (0x10756100, 0x10756274, 92, '8bb51156a0b7ae0b'),
    (0x10756280, 0x10756310, 36, '752720f99aa27ca3'), (0x10756310, 0x10756520, 132, 'ec9616b4a6d5e9c3'),
    (0x10756520, 0x10756554, 13, '3b4f33fec0e39ed9'), (0x10756560, 0x10756594, 13, 'bd42bd4fd448b5b3'),
    (0x107565a0, 0x1075662c, 35, 'bd8ed3e888e2a96d'), (0x10756630, 0x107566a4, 29, '18a73f0b905afa84'),
    (0x107566b0, 0x1075681c, 91, '33f61a16764376d3'), (0x10756820, 0x107569fc, 119, '9c99b7910b5e0e9c'),
    (0x10756a00, 0x10756b5c, 87, '8641094b12814071'), (0x10756b60, 0x10756bfc, 39, '97a93b586d990669'),
    (0x10756c00, 0x10756c8c, 35, '6308c1a2b6a9a473'), (0x10756c90, 0x10756cbc, 11, 'a97eb3c511c14e31'),
    (0x10756cc0, 0x10756cec, 11, 'c86ffd561b822821'), (0x10756cf0, 0x10756d14, 9, '31dd3422a3ea6db6'),
    (0x10756d20, 0x10756fb4, 165, '25535de36de86436'), (0x10756fc0, 0x10756fdc, 7, 'c76e6fdff7cf725b'),
    (0x10756fe0, 0x10756ffc, 7, 'e57ba376ff34c20c'), (0x10757000, 0x1075701c, 7, '21953abfb35daca6'),
    (0x10757020, 0x10757074, 21, 'cc9e3ea9ee525eb0'), (0x10757080, 0x107570d8, 22, '28b9fc287047ccff'),
    (0x107570e0, 0x10757144, 25, '53d0982f0c23a996'), (0x10757150, 0x10757198, 18, 'b33fff7e025eacd7'),
    (0x107571a0, 0x107571e8, 18, '84303e7f7bc4aa5a'), (0x107571f0, 0x10757288, 38, 'db6d42eb9b475040'),
    (0x10757290, 0x10757420, 100, '2a412be2cca5a22c'), (0x10757420, 0x10757468, 18, 'f5e091f8c5c2f6ae'),
    (0x10757470, 0x10757488, 6, '2b703046d17279c3'), (0x10757490, 0x107574a0, 4, '627b0e7cd90b8346'),
    (0x107574a0, 0x107574b8, 6, '991141d8b3f197ab'), (0x107574c0, 0x107574e8, 10, 'babee0c08521ea63'),
    (0x107574f0, 0x1075750c, 7, '757fca9c1c7e28c6'), (0x10757510, 0x10757558, 18, '2754ba82d720b057'),
    (0x10757560, 0x10757580, 8, '9b7018f666f349b7'), (0x10757580, 0x1075764c, 51, 'cd742952c1900874'),
    (0x10757650, 0x1075778c, 79, '749252d8777daf36'), (0x10757790, 0x10757ab0, 197, 'dfc1ce0b87e4277b'),
    (0x10757ab0, 0x10757bcc, 71, '6b7e0263b1c09d7f'), (0x10757bd0, 0x10757ca0, 52, '7cac18bef7c32e1a'),
    (0x10757ca0, 0x10757dc0, 71, '9acbb15b1ff3e201'), (0x10757dc0, 0x10757fac, 123, '2742c4fa2790bd1b'),
    (0x10757fb0, 0x107580cc, 70, 'd451b257c4bb0309'), (0x107580d0, 0x10758268, 102, 'f07cd0170ef632f7'),
    (0x10758270, 0x107583b0, 80, '9ea6a1bfc8ac13af'), (0x107583b0, 0x10758474, 49, 'fe6bc4e12422edcd'),
    (0x10758480, 0x1075854c, 51, 'ac4b806547884b6e'), (0x10758550, 0x1075856c, 7, '5a0898485401b5d6'),
    (0x10758570, 0x107585b4, 16, 'edff37d2963d79fa'), (0x107585c0, 0x1075860c, 18, '2e55423de2b923fd'),
    (0x10758610, 0x10758668, 22, '2311395f1c555930'), (0x10758670, 0x10758698, 10, 'afddc0d137924b09'),
    (0x107586a0, 0x10758704, 25, '7be165c8eaadbd9e'), (0x10758710, 0x10758790, 32, 'be67b4a111221d1b'),
    (0x10758790, 0x10758ad8, 209, 'e08f11b0070eeb1e'), (0x10758ae0, 0x10758b58, 30, '68ecbcbd920ce9ca'),
    (0x10758b60, 0x10758b90, 12, '3dd7b0d1f18cce4d'), (0x10758b90, 0x10758bc4, 13, 'a933ccc6d5b4d47f'),
    (0x10758bd0, 0x10758c00, 12, '4ca675c6ed7adf49'), (0x10758c00, 0x10758c34, 13, '917f3629ccccb6ff'),
    (0x10758c40, 0x10758c74, 13, 'acb9e92b6de659e2'), (0x10758c80, 0x10758cb4, 13, '2c691433c6750ba2'),
    (0x10758cc0, 0x10758d6c, 43, 'baf3362915b2bf57'), (0x10758d70, 0x10758ddc, 27, '916048350bfa2cd3'),
    (0x10758de0, 0x10758e34, 21, 'a4e19d5e05e4e197'), (0x10758e40, 0x10758edc, 39, '0f0725ca7a2fc2ba'),
    (0x10758ee0, 0x10758f94, 45, '7b5f3b53b46c90dc'), (0x10758fa0, 0x107590a4, 65, '2870d1c749f582a7'),
    (0x107590b0, 0x10759184, 53, '19e1b6df65f21dc7'), (0x10759190, 0x107591ac, 7, '953f0d2a54fd7711'),
    (0x107591b0, 0x10759218, 26, '24af0c2dbdfbd403'), (0x10759220, 0x10759230, 4, '361b4b7504c0723c'),
    (0x10759230, 0x10759240, 4, '8a5559a5781f999d'), (0x10759240, 0x10759250, 4, 'e87c5a18dea29a16'),
    (0x10759250, 0x10759430, 120, '316f5f7e47b82537'), (0x10759430, 0x10759514, 57, '1d834c5bd81e92e5'),
    (0x10759520, 0x107595a0, 32, '25d1f930ad32663e'), (0x107595a0, 0x10759654, 45, '8dfa0d8efb96642b'),
    (0x10759660, 0x107596e4, 33, 'dbb6d325812c43c9'), (0x107596f0, 0x107596f4, 1, '11c636d251d70990'),
    (0x10759700, 0x10759724, 9, 'd9591a5ddc7af155'), (0x10759730, 0x10759764, 13, 'c1f9dc127ceb0a2b'),
    (0x10759770, 0x107597a4, 13, '17cfd7f3100204d2'), (0x107597b0, 0x1075980c, 23, 'f2c40ec5b979a531'),
    (0x10759810, 0x10759834, 9, 'ffed0001405931e3'), (0x10759840, 0x107598d8, 38, '026a8e227a7c4f7c'),
    (0x107598e0, 0x10759ad4, 125, 'af686047d1618b23'), (0x10759ae0, 0x10759dbc, 183, 'dab125f023913467'),
    (0x10759dc0, 0x10759e28, 26, '26f9a4533fa2e22e'), (0x10759e30, 0x10759ee8, 45, '58b2d38f1e2b22e0'),
    (0x10759ef0, 0x10759fa0, 44, '48a8ba0d037f47a1'), (0x10759fa0, 0x1075a040, 40, '6cae077f561b0a22'),
    (0x1075a040, 0x1075a150, 67, 'b95697572a724198'), (0x1075a150, 0x1075a19c, 19, '7f6c750a8a6ce2df'),
    (0x1075a1a0, 0x1075a1bc, 7, '3cece4f0405342e2'), (0x1075a1c0, 0x1075a1dc, 7, '146c716df0ce1a1f'),
    (0x1075a1e0, 0x1075a3a0, 111, 'fe4f37b83763aa53'), (0x1075a3a0, 0x1075a3d0, 12, 'b0f0dfdc05d0b657'),
    (0x1075a3d0, 0x1075a408, 14, '32981cef843f2463'), (0x1075a410, 0x1075a448, 14, '03b207c5c939ef94'),
    (0x1075a450, 0x1075a480, 12, '3058634de163db4b'), (0x1075a480, 0x1075a4b8, 14, '477c62ec76fef2b2'),
    (0x1075a4c0, 0x1075a4f8, 14, 'ac4f56ace7996a3e'), (0x1075a500, 0x1075a964, 276, '95bc32076cfa846d'),
    (0x1075a970, 0x1075ac38, 178, 'de35f72612319b17'), (0x1075ac40, 0x1075acc8, 34, '7966f7c66edd3ff7'),
    (0x1075acd0, 0x1075ad58, 34, 'f1e295fd4f032256'), (0x1075ad60, 0x1075ae30, 52, '1321dba11a8083b7'),
    (0x1075ae30, 0x1075af60, 76, 'dd1e4ccc047ba397'), (0x1075af60, 0x1075b970, 644, '1c768a0089d33798'),
    (0x1075b970, 0x1075bb94, 137, 'f95ed7d53e974a47'), (0x1075bba0, 0x1075bc90, 60, 'ed2a8eeac1fa6cff'),
    (0x1075bc90, 0x1075c0e8, 277, '205ed96fc0fb0377'), (0x1075c0f0, 0x1075c824, 461, '6f2f8241b858ae97'),
    (0x1075c830, 0x1075c8c0, 36, '6b8f531a2b6c7b7c'), (0x1075c8c0, 0x1075c97c, 47, '883415ed5fbc3939'),
    (0x1075c980, 0x1075ca50, 52, '1f244de39eb4785c'), (0x1075ca60, 0x1075cbbc, 87, '1b3d759a557c7aa5'),
    (0x1075cbc0, 0x1075e0bc, 1338, 'fe0b9a9bc59d0759'), (0x1075e0c0, 0x1075e1a0, 56, 'cdbac53a67a70692'),
    (0x1075e1a0, 0x1075e31c, 95, 'fc56901655b13af0'), (0x1075e320, 0x1075e958, 398, 'a75ab4dfd0386305'),
    (0x1075e960, 0x1075eb0c, 107, 'f4ed1a99d282da48'), (0x1075eb10, 0x1075ecdc, 115, 'dedbb811de1f5cca'),
    (0x1075ece0, 0x1075eef0, 132, 'da9c5f2cd62a634b'), (0x1075eef0, 0x1075f868, 606, '7c415df271af60f8'),
    (0x1075f870, 0x1075f978, 66, '43e3ec60d48522db'), (0x1075f980, 0x1075fa08, 34, 'addf5322d3f5b784'),
    (0x1075fa10, 0x10760e30, 1286, '646b65a907ca3697'), (0x10760e30, 0x10760f44, 69, '88455a302878e289'),
    (0x10760f50, 0x10761064, 69, '9722c3f1c4918411'), (0x10761070, 0x10761268, 126, '6e598cde415e7d43'),
    (0x10761270, 0x107614c0, 148, 'd02ccb40541c2fea'), (0x107614c0, 0x10761730, 151, 'ff8bec691aaab880'),
    (0x10761730, 0x107617ac, 31, '667196286a31a9e5'), (0x107617b0, 0x10761d44, 351, 'c2e994beca6ae3ea'),
    (0x10761d50, 0x10761eb8, 90, '1fa55aa23f0a4e89'), (0x10761ec0, 0x107620d0, 131, '07e5f42d1eb2ad82'),
    (0x107620d0, 0x107623b8, 186, 'd2f83d9ec1b6118a'), (0x107623c0, 0x10762e2c, 666, 'c908c5dc62381a0b'),
    (0x10762e30, 0x10762f40, 67, 'cde913d57fbfbd66'), (0x10762f40, 0x10762fd0, 36, 'b444e9cd48b3889d'),
    (0x10762fd0, 0x1076304c, 31, 'c56002caaa3c7b98'), (0x10763050, 0x107630d8, 34, 'df4fe86c15a27f8f'),
    (0x107630e0, 0x107630f0, 4, '1942a7148f421a2c'), (0x107630f0, 0x107631b8, 50, '48381f9f87d6ea89'),
    (0x107631c0, 0x1076327c, 47, '99241bfa7bc30bb7'), (0x10763280, 0x1076328c, 3, '8cd56b6852e0897d'),
    (0x10763290, 0x107632a0, 4, 'e86610bb09c1e151'), (0x107632a0, 0x10763350, 44, '2755bb5ae07f0a71'),
    (0x10763350, 0x1076338c, 15, 'bec17c554aaae898'), (0x10763390, 0x1076381c, 291, 'c185d7e9f7bc8f63'),
    (0x10763820, 0x10763840, 8, '05b5f5ee295ccc77'), (0x10763840, 0x10763888, 18, '15cc6be57898165e'),
    (0x10763890, 0x107638bc, 11, 'beb95cb69f0b7a72'), (0x107638c0, 0x10763ea4, 376, '25fae3470b7781d5'),
    (0x10763eb0, 0x107642d4, 265, '591e83ae989604e3'), (0x107642e0, 0x107643c8, 58, 'da478f5431cf3656'),
    (0x107643d0, 0x107644c0, 59, 'aee6244474038248'), (0x107644c0, 0x107645a0, 56, 'b75deb37c2094ce4'),
    (0x107645a0, 0x1076460c, 27, 'e51446a3c04e725e'), (0x10764610, 0x107649a0, 226, 'eedbfc0f69ae536a'),
    (0x107649a0, 0x10764b08, 89, 'fa27078f24dc826e'), (0x10764b10, 0x10764be8, 54, '40664dad64441e24'),
    (0x10764bf0, 0x10764d10, 72, '6f67f3ff4ea03274'), (0x10764d10, 0x10764e48, 78, '686df3fd4e9240c1'),
    (0x10764e50, 0x10764f58, 66, '43cb52b2be22e606'), (0x10764f60, 0x1076505c, 62, '17876294f3afea4e'),
    (0x10765060, 0x10765728, 433, '2b38c5ff06b550f8'), (0x10765730, 0x10765780, 19, '59752b7b156cda73'),
    (0x10765780, 0x10765810, 36, 'd0d2eeb2c85e1978'), (0x10765810, 0x1076642c, 756, '136cee12a92277a8'),
    (0x10766430, 0x1076649c, 27, '837354610a39de48'), (0x107664a0, 0x10766768, 176, 'ee441d8ec63ecf4b'),
    (0x10766770, 0x107668c4, 84, 'e8c3be1f044dc23d'), (0x107668d0, 0x10766aa8, 117, '6568684c3fb79ee4'),
    (0x10766ab0, 0x10766cbc, 131, '7c2c5b7d332737a8'), (0x10766cc0, 0x10766d24, 25, '7ee3692f8fa7a3f6'),
    (0x10766d30, 0x10766db4, 33, '00edbae96a5f776c'), (0x10766dc0, 0x107670d4, 197, 'a5b8cc91a4f57d4d'),
    (0x107670e0, 0x10767188, 42, 'a801e1f520e2e6c8'), (0x10767190, 0x1076742c, 167, '9e6501785d781457'),
    (0x10767430, 0x10767460, 12, 'be903f73c34d95b7'), (0x10767460, 0x107675c0, 87, '90befeead1eba8b5'),
    (0x107675c0, 0x10767684, 49, 'c613c13378ecf033'), (0x10767690, 0x1076771c, 35, 'd76521fc64f5520f'),
    (0x10767720, 0x10767814, 61, 'b06e611c8ba1c1e8'), (0x10767820, 0x10767894, 28, 'c5ee7bee86336866'),
    (0x107678a0, 0x107679f4, 85, 'f206cfcb70a47e20'), (0x10767a00, 0x10767bb0, 108, 'd59d13a3ec035d7c'),
    (0x10767bb0, 0x10767ce0, 76, '2dce1fe08de3d6e0'), (0x10767ce0, 0x10767d34, 21, '35af70b3a6d96ab4'),
    (0x10767d40, 0x10767dbc, 31, 'e4215acc432d795e'), (0x10767dc0, 0x10767e40, 32, '6bb91661067f86ad'),
    (0x10767e40, 0x10767eb0, 28, '857fcee0ddc25f02'), (0x10767eb0, 0x10767f20, 28, '47b0c8b07837ec75'),
    (0x10767f20, 0x10768074, 85, '6b3102e8b6929a18'),
)
ENTRIES = tuple(a for a, _, _, _ in CONTRACTS)
# The routed entries: block util (the six fixed-point helpers, the sort and the distance) and block stream (the three readers).
ROUTED = frozenset({0x10767ce0, 0x10767d40, 0x10767dc0, 0x10767e40, 0x10767eb0, 0x10767f20,
                    0x10762f40, 0x10762fd0, 0x10763050,
                    0x10767460, 0x107675c0, 0x10767690, 0x10767720, 0x10767820, 0x107678a0, 0x10767a00, 0x10767bb0,
                    # t2ktt_head
                    0x107630e0, 0x107630f0, 0x107631c0, 0x10763280, 0x10763290, 0x107632a0, 0x10763350, 0x10763390, 0x10763820, 0x10763840,
                    0x10763890, 0x107638c0, 0x10763eb0, 0x107642e0, 0x107643d0, 0x107644c0, 0x107645a0,
                    # glyph
                    0x1075b970, 0x1075bba0, 0x1075bc90, 0x1075c0f0, 0x1075c830, 0x1075c8c0,
                    # truetype_tables
                    0x10764610, 0x107649a0, 0x10764b10, 0x10764bf0, 0x10764d10, 0x10764e50, 0x10764f60, 0x10765060, 0x10765730, 0x10765780,
                    0x1075e0c0, 0x1075e1a0, 0x1075e320,
                    0x10761070,
                    # truetype_glyph
                    0x10765810, 0x10766430, 0x107664a0, 0x10766770, 0x107668d0, 0x10766ab0, 0x10766cc0, 0x10766d30, 0x10766dc0, 0x107670e0,
                    0x10767190, 0x10767430,
                    # t2ksc
                    0x10761270, 0x107614c0, 0x10761730, 0x107617b0, 0x10761d50, 0x10761ec0, 0x107620d0, 0x107623c0, 0x10762e30,
                    # fnt_c
                    0x10759250, 0x10759430, 0x10759520, 0x107595a0, 0x10759660, 0x107596f0, 0x10759700, 0x10759730, 0x10759770, 0x107597b0, 0x10759810,
                    0x10759840, 0x107598e0, 0x10759ae0, 0x10759dc0, 0x10759e30, 0x10759ef0, 0x10759fa0, 0x1075a040, 0x1075a150, 0x1075a1a0, 0x1075a1c0,
                    0x1075a1e0, 0x1075a3a0, 0x1075a3d0, 0x1075a410, 0x1075a450, 0x1075a480, 0x1075a4c0, 0x1075a500, 0x1075a970,
                    # fnt_b
                    0x10757fb0, 0x107580d0, 0x10758270, 0x107583b0, 0x10758480, 0x10758550, 0x10758570, 0x107585c0, 0x10758610, 0x10758670, 0x107586a0,
                    0x10758710, 0x10758790, 0x10758ae0, 0x10758b60, 0x10758b90, 0x10758bd0, 0x10758c00, 0x10758c40, 0x10758c80, 0x10758cc0, 0x10758d70,
                    0x10758de0, 0x10758e40, 0x10758ee0, 0x10758fa0, 0x107590b0, 0x10759190, 0x107591b0, 0x10759220, 0x10759230, 0x10759240,
                    # fnt_a
                    0x107559a0, 0x107559d0, 0x10755a20, 0x10755a60, 0x10755ab0, 0x10755b00, 0x10755b50, 0x10755b60,
                    0x10755be0, 0x10755cc0, 0x10755e00, 0x10755e30, 0x10755e60, 0x10755ec0, 0x10755f20, 0x10755f30,
                    0x10755f40, 0x10755f60, 0x10756020, 0x10756030, 0x107560e0, 0x10756100, 0x10756280, 0x10756310,
                    0x10756520, 0x10756560, 0x107565a0, 0x10756630, 0x107566b0, 0x10756820, 0x10756a00, 0x10756b60,
                    0x10756c00, 0x10756c90, 0x10756cc0, 0x10756cf0, 0x10756d20, 0x10756fc0, 0x10756fe0, 0x10757000,
                    0x10757020, 0x10757080, 0x107570e0, 0x10757150, 0x107571a0, 0x107571f0, 0x10757290, 0x10757420,
                    0x10757470, 0x10757490, 0x107574a0, 0x107574c0, 0x107574f0, 0x10757510, 0x10757560, 0x10757580,
                    0x10757650, 0x10757790, 0x10757ab0, 0x10757bd0, 0x10757ca0, 0x10757dc0,
                    # ghints
                    0x1075ac40, 0x1075acd0, 0x1075ad60, 0x1075ae30, 0x1075af60,
                    # hstrat97
                    0x1075c980, 0x1075ca60, 0x1075cbc0,
                    # t2k_core
                    0x1075e960, 0x1075eb10, 0x1075ece0, 0x1075eef0, 0x1075f870, 0x1075f980, 0x1075fa10, 0x10760e30, 0x10760f50,
                    # autogrid_a
                    0x10751350, 0x10751b10, 0x10751bd0, 0x10751eb0, 0x10751fd0, 0x107522b0, 0x107524b0, 0x10752990, 0x10753110,
                    # autogrid_b
                    0x10753560, 0x107536b0, 0x107537c0, 0x10753980, 0x10753a80, 0x10753ab0, 0x10753bf0, 0x10753c30, 0x10753c70, 0x10753cd0,
                    0x10753d20, 0x10753d50, 0x10753d60, 0x10753d70, 0x10753e70, 0x10753f70, 0x10753fa0, 0x10753fd0, 0x10754050, 0x107540d0,
                    0x10754130, 0x10754190, 0x10754370, 0x10754550, 0x107545b0, 0x10754610, 0x10754810, 0x107548d0, 0x10754990, 0x10754cd0,
                    0x10754f70, 0x10754ff0, 0x107551c0, 0x107552b0, 0x107555d0, 0x10755840, 0x10755930})
# Per unit: routines, instructions (the inventory the brief's block list is built on).
UNIT_COUNTS = {'AUTOGRID': (46, 4432), 'FNT': (125, 5069), 'GHINTS': (5, 840), 'GLYPH': (6, 1018), 'HSTRAT97': (3, 1477),
               'STYLE': (3, 549), 'T2K': (9, 2484), 'T2KEXTRA': (1, 126), 'T2KSC': (9, 1821), 'T2KSTRM': (3, 101),
               'T2KTT': (8, 458), 'TRUETYPE': (31, 3780), 'TSIMEM': (8, 529), 'UTIL': (6, 225)}
# The jump tables: three `bctr`, in three routines of the FNT unit (the opcode handlers 0x10757080, 0x10759250, 0x10759430).
BCTR = {0x107570b0: 0x10757080, 0x10759294: 0x10759250, 0x10759474: 0x10759430}
LABELS = 97  # the jump tables' words: 17 for 0x10757080 and 80 for the other two
ASSEMBLY_TARGET = re.compile(r'^(b|bl|b[a-z]+) (0x[0-9a-f]+)$')
SLOT = re.compile(r'^lwz r\d+,(-?0x[0-9a-f]+)\(r2\)$')


def require_export(path):
    if not (path / 'instructions.sqlite').is_file():
        raise RuntimeError('Private font engine export unavailable; inventory cannot be certified')


class FontEngineAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export(EXPORT)
        cls.program = json.loads((EXPORT / 'program.json').read_text())
        cls.segments = [(s['address'], (EXPORT / s['file']).read_bytes()) for s in cls.program['segments'] if 'file' in s]
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            cls.records = routine_records(db, *SPAN)
            cls.branches = [(a, m.group(1), int(m.group(2), 16)) for a, text in db.execute(
                "SELECT address, json_extract(body, '$.assembly') FROM instructions "
                "WHERE json_extract(body, '$.assembly') LIKE 'b% 0x1075%' OR json_extract(body, '$.assembly') LIKE 'b% 0x1076%'")
                for m in [ASSEMBLY_TARGET.match(text)] if m]
            cls.glue_sites = [a for (a,) in db.execute("SELECT address FROM instructions WHERE address >= ? AND address < ? AND "
                                                       "json_extract(body, '$.assembly') = 'bl 0x10781320'", SPAN)]
            cls.loads = {}
            for a, text in db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions "
                                      "WHERE json_extract(body, '$.assembly') LIKE 'lwz r%(r2)'"):
                if m := SLOT.match(text):
                    cls.loads.setdefault(int(m.group(1), 16), []).append(a)
        cls.by_address = {r['address']: r for r in cls.records}

    def word(self, address):
        for start, contents in self.segments:
            if start <= address and address + 4 <= start + len(contents):
                return struct.unpack_from('>I', contents, address - start)[0]
        raise AssertionError(f'no data at {address:x}')

    def string(self, address):
        for start, contents in self.segments:
            if start <= address < start + len(contents):
                return contents[address - start:contents.index(b'\0', address - start)].decode()
        raise AssertionError(f'no data at {address:x}')

    def unit_routines(self, name):
        start, end = audit.MODULES[name]
        return [(a, b) for a, b in audit.ROUTINES if start <= a < end]

    def test_exact_entries_and_original_digests(self):
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in CONTRACTS))
        self.assertEqual(audit.FONT_ENGINE_ENTRIES, ENTRIES)
        self.assertEqual(len(ENTRIES), 263)
        self.assertEqual(len(self.records), 22909)
        self.assertEqual(sum(count for _, _, count, _ in CONTRACTS), 22909)
        for start, end, count, expected in CONTRACTS:
            body = [r for r in self.records if start <= r['address'] < end]
            self.assertEqual(len(body), count)
            self.assertEqual(digest(body), expected)
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_font_engine_index(db, complete=False)

    def test_units_tile_the_span_and_their_pools_name_their_files_in_link_order(self):
        units = list(audit.MODULES.items())
        self.assertEqual(units[0][1][0], SPAN[0])
        for (_, (_, end)), (_, (start, _)) in zip(units, units[1:]):
            self.assertEqual(end, start)
        self.assertGreaterEqual(units[-1][1][1], SPAN[1])
        self.assertLess(units[-1][1][1] - SPAN[1], 0x10)  # only alignment padding after the last routine
        for name, (lo, hi) in units:
            self.assertEqual(audit.unit_of(lo), name)
            routines = self.unit_routines(name)
            self.assertEqual((len(routines), sum(len([r for r in self.records if a <= r['address'] < b]) for a, b in routines)), UNIT_COUNTS[name], name)
            self.assertEqual(routines[0][0], lo, name)  # a unit begins at an entry
        previous = 0
        for name, (source, name_at, slot, first) in audit.POOLS.items():
            self.assertEqual(self.string(name_at), source)
            self.assertEqual(self.word(TOC + slot), first, name)  # the slot points at the unit's first assertion string
            self.assertTrue(first < name_at < first + 0x20, name)  # the name follows it
            self.assertGreater(name_at, previous)  # link order
            previous = name_at
            lo, hi = audit.MODULES[name]
            users = [a for a in self.loads[slot]]
            self.assertTrue(users and all(lo <= a < hi for a in users), name)  # every load of the pool is inside the unit
        # The three units without a pool: no routine of theirs loads a pool slot of another unit.
        pool_slots = {slot for _, _, slot, _ in audit.POOLS.values()}
        for name in ('FNT', 'STYLE', 'UTIL'):
            lo, hi = audit.MODULES[name]
            for slot in pool_slots:
                self.assertFalse([a for a in self.loads[slot] if lo <= a < hi], (name, slot))

    def test_every_instruction_and_unreachable_word_accounted(self):
        covered = set()
        for start, end in audit.ROUTINES:
            covered.update(range(start, end, 4))
        have = set(self.by_address)
        self.assertEqual(have, covered - audit.UNREACHABLE)
        self.assertEqual(len(audit.UNREACHABLE), 78)
        self.assertTrue(audit.UNREACHABLE <= covered)
        # An unreachable word follows an unconditional branch or a return (an alignment no-op).
        for address in audit.UNREACHABLE:
            before = address - 4
            while before in audit.UNREACHABLE:
                before -= 4
            self.assertRegex(self.by_address[before]['assembly'], r'^(b |blr)')
        for (_, end), (start, _) in zip(audit.ROUTINES, audit.ROUTINES[1:]):
            self.assertLessEqual(end, start)
            self.assertLessEqual(start - end, 0x10)  # padding between routines only

    def test_the_api_is_the_calls_from_font_cpp_and_nothing_else_names_the_engine(self):
        entries = set(ENTRIES)
        outside = [(a, kind, target) for a, kind, target in self.branches if SPAN[0] <= target < SPAN[1] and not SPAN[0] <= a < SPAN[1]]
        self.assertEqual({target for _, _, target in outside}, set(audit.API))
        self.assertTrue(all(kind == 'bl' for _, kind, _ in outside))
        for site, _, target in outside:
            self.assertIn(target, entries)
            self.assertTrue(0x1074f470 <= site < 0x10751350 and any(site >= caller for caller in audit.API[target]), hex(site))
        self.assertEqual(len(outside), 13)
        # No data word names the engine except descriptors (code word, TOC word) of entries and the jump tables' labels.
        found = {}
        for start, contents in self.segments:
            first = max(start, CODE_END)
            for address in range((first + 3) & ~3, start + len(contents) - 3, 4):
                value = struct.unpack_from('>I', contents, address - start)[0]
                if SPAN[0] <= value < SPAN[1]:
                    found[address] = value
        descriptors = {a: v for a, v in found.items() if self.word(a + 4) == TOC and v in entries}
        rest = {a: v for a, v in found.items() if a not in descriptors}
        # A word that is not a multiple of 4 is not a code address: one such coincidence lies in the data (0x108f0d60).
        self.assertEqual({a: v for a, v in rest.items() if v % 4}, {0x108f0d60: 0x107649f9})
        labels = {a: v for a, v in rest.items() if not v % 4}
        self.assertTrue(all(0x108fbfb4 <= a < 0x108fc138 for a in labels))
        self.assertEqual(len(descriptors), 131)
        self.assertEqual(len(set(descriptors.values())), 131)
        for value in labels.values():
            routine = max(e for e in ENTRIES if e <= value)
            self.assertIn(routine, set(BCTR.values()), hex(value))
        self.assertEqual(len(labels), LABELS)
        self.assertEqual(sorted({max(e for e in ENTRIES if e <= v) for v in labels.values()}), sorted(set(BCTR.values())))
        # The only indirect transfers: the glue 0x10781320's callers (66 `bl`) and the three `bctr`.
        self.assertEqual(len(self.glue_sites), 66)
        self.assertEqual({a for a, r in self.by_address.items() if r['assembly'] == 'bctr'}, set(BCTR))

    def test_branches_stay_inside_their_routine_and_calls_name_entries(self):
        entries = set(ENTRIES)
        ends = dict(audit.ROUTINES)
        for site, kind, target in self.branches:
            if not SPAN[0] <= site < SPAN[1]:
                continue
            routine = max(e for e in ENTRIES if e <= site)
            if kind == 'bl':
                if SPAN[0] <= target < SPAN[1]:
                    self.assertIn(target, entries, hex(site))
                continue
            self.assertTrue(routine <= target < ends[routine], (hex(site), hex(target)))  # no tail call, no branch out
        for label_site, routine in BCTR.items():
            self.assertTrue(routine <= label_site < ends[routine])

    def test_pointer_only_entries_are_loaded_descriptors(self):
        called = {target for _, kind, target in self.branches if kind == 'bl' and SPAN[0] <= target < SPAN[1]}
        self.assertEqual(set(ENTRIES) - called, audit.POINTER_ONLY)
        self.assertEqual(len(audit.POINTER_ONLY), 127)
        self.assertEqual(sum(count for e, _, count, _ in CONTRACTS if e in audit.POINTER_ONLY), 4760)
        descriptors = {}
        for start, contents in self.segments:
            for address in range((max(start, CODE_END) + 3) & ~3, start + len(contents) - 7, 4):
                code, toc = struct.unpack_from('>II', contents, address - start)
                if toc == TOC and code in audit.POINTER_ONLY:
                    descriptors[code] = address
        self.assertEqual(set(descriptors), set(audit.POINTER_ONLY))
        loaded = {self.word(TOC + slot): slot for slot in self.loads}
        for entry, address in descriptors.items():
            self.assertIn(address, loaded, hex(entry))  # a TOC slot holds the descriptor: some code loads it
            users = self.loads[loaded[address]]
            self.assertTrue(any(SPAN[0] <= u < SPAN[1] or 0x10750000 <= u < SPAN[0] for u in users), hex(entry))

    def test_exactly_the_routed_blocks_are_routed_and_hooked_and_the_hook_checks_fail_when_they_should(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, generate_page
        pinned = {a: (b, d) for a, b, _, d in CONTRACTS}
        self.assertTrue(audit.COMPLETE)
        listed = audit.rewritten(drafts=True)
        for entry, end, expected in listed:
            self.assertEqual(pinned[entry], (end, expected))
        self.assertEqual({entry for entry, _, _ in listed}, ROUTED)
        self.assertEqual({entry for entry, _, _ in audit.rewritten()}, ROUTED)
        self.assertEqual(set(audit.FONT_ENGINE_HOOKED), ROUTED)
        for entry in ENTRIES:
            owners = [owner for at, owner in OWNER_REPLACEMENTS if at == entry]
            self.assertEqual(owners, [audit.OWNER] if entry in ROUTED else [], hex(entry))
        self.assertEqual([owner for at, owner in SEMANTIC_REPLACEMENTS if at in ENTRIES], [])
        with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
            audit.require_font_engine_index(db)
            # The completeness check refuses a set with one entry short of production (a draft).
            with tempfile.TemporaryDirectory() as directory:
                candidate = Path(directory) / 'routines.inc'
                first, end, _, pinned = CONTRACTS[0]
                lines = ''.join(f'FONT_ENGINE_ROUTINE(0x{a:x}, 0x{b:x}, R3, "{d}", name)\n' for a, b, _, d in CONTRACTS)
                candidate.write_text(lines)
                audit.require_font_engine_index(db, candidate, complete=True)
                candidate.write_text(lines.replace(f'FONT_ENGINE_ROUTINE(0x{first:x},', f'FONT_ENGINE_DRAFT(0x{first:x},', 1))
                with self.assertRaisesRegex(ValueError, 'requires all 263'):
                    audit.require_font_engine_index(db, candidate, complete=True)
        selected = set(self.by_address)
        pages = sorted({entry >> 12 for entry in ROUTED} | {0x10763})  # the routed pages and one with unrouted entries only
        for page in pages:
            part = [r for r in self.records if r['address'] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.program['userops'])
            audit.require_font_engine_hooks(page, part, normal)
            on_page = [entry for entry in ENTRIES if entry >> 12 == page]
            hooked = [entry for entry in on_page if entry in ROUTED]
            self.assertEqual('#include "font_engine_adapter.hpp"' in normal, bool(hooked), hex(page))
            for entry in on_page:
                hook = audit.font_engine_hook(entry)
                fragment = normal.split(f'i_{entry:x}:', 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
                step = f's.step(0x{entry:x});'
                if entry not in ROUTED:
                    self.assertNotIn(hook, normal)
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_font_engine_hooks(page, part, normal.replace(step, hook + '\n' + step, 1))
                    with mock.patch.object(audit, 'FONT_ENGINE_HOOKED', audit.FONT_ENGINE_HOOKED + (entry,)):
                        with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                            audit.require_font_engine_hooks(page, part, normal)
                    continue
                self.assertEqual(normal.count(hook), 1)
                self.assertIn(hook, fragment)
                self.assertLess(fragment.index(hook), fragment.index(step))
                moved = normal.replace(hook, '').replace(step, step + '\n' + hook, 1)
                for broken in (normal.replace(hook, ''), normal.replace(hook, hook + '\n' + hook), moved):
                    with self.assertRaisesRegex(ValueError, 'Missing or misplaced'):
                        audit.require_font_engine_hooks(page, part, broken)
                with mock.patch.object(audit, 'FONT_ENGINE_HOOKED', tuple(e for e in audit.FONT_ENGINE_HOOKED if e != entry)):
                    with self.assertRaisesRegex(ValueError, 'Unexpected'):
                        audit.require_font_engine_hooks(page, part, normal)

    def test_missing_export_stale_digest_and_bad_lines_are_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, 'cannot be certified'):
                require_export(Path(directory))
            candidate = Path(directory) / 'routines.inc'
            start, end, _, pinned = CONTRACTS[100]
            line = f'FONT_ENGINE_DRAFT(0x{start:x}, 0x{end:x}, R3, "{pinned}", name)\n'
            candidate.write_text(line)
            self.assertEqual(audit.rewritten(candidate), [])
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])
            with closing(sqlite3.connect(EXPORT / 'instructions.sqlite')) as db:
                audit.require_font_engine_index(db, candidate, complete=False)
                candidate.write_text(line.replace(pinned, '0000000000000000'))
                with self.assertRaisesRegex(ValueError, 'changed in the export'):
                    audit.require_font_engine_index(db, candidate, complete=False)
            with closing(sqlite3.connect(':memory:')) as db:
                db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT NOT NULL)')
                candidate.write_text(line)
                with self.assertRaisesRegex(ValueError, 'Missing font engine instructions'):
                    audit.require_font_engine_index(db, candidate, complete=False)
            candidate.write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'listed twice'):
                audit.rewritten(candidate, drafts=True)
            candidate.write_text(line.replace(f'0x{end:x}', f'0x{end + 4:x}'))
            with self.assertRaisesRegex(ValueError, 'is not a routine'):
                audit.rewritten(candidate, drafts=True)
            block = Path(directory) / 'font_engine_routines_block.inc'
            block.write_text(line)
            candidate.write_text('#include "font_engine_routines_block.inc"\n')
            self.assertEqual(audit.rewritten(candidate, drafts=True), [(start, end, pinned)])


if __name__ == '__main__':
    unittest.main()
