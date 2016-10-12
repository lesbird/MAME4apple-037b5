# uncomment the following lines to include a CPU core
CPUS+=Z80@
#CPUS+=Z80GB@
CPUS+=8080@
CPUS+=8085A@
CPUS+=M6502@
CPUS+=M65C02@
#CPUS+=M65SC02@
#CPUS+=M65CE02@
#CPUS+=M6509@
CPUS+=M6510@
#CPUS+=M6510T@
#CPUS+=M7501@
#CPUS+=M8502@
#CPUS+=M4510@
CPUS+=N2A03@
CPUS+=H6280@
CPUS+=I86@
#CPUS+=I88@
CPUS+=I186@
#CPUS+=I188@
#CPUS+=I288@
CPUS+=V20@
CPUS+=V30@
CPUS+=V33@
CPUS+=I8035@
CPUS+=I8039@
CPUS+=I8048@
CPUS+=N7751@
CPUS+=M6800@
CPUS+=M6801@
CPUS+=M6802@
CPUS+=M6803@
CPUS+=M6808@
CPUS+=HD63701@
CPUS+=NSC8105@
CPUS+=M6805@
CPUS+=M68705@
CPUS+=HD63705@
CPUS+=HD6309@
CPUS+=M6809@
CPUS+=KONAMI@
CPUS+=M68000@
CPUS+=M68010@
CPUS+=M68EC020@
CPUS+=M68020@
CPUS+=T11@
CPUS+=S2650@
CPUS+=TMS34010@
#CPUS+=TMS9900@
#CPUS+=TMS9940@
CPUS+=TMS9980@
#CPUS+=TMS9985@
#CPUS+=TMS9989@
#CPUS+=TMS9995@
#CPUS+=TMS99105A@
#CPUS+=TMS99110A@
CPUS+=Z8000@
CPUS+=TMS320C10@
CPUS+=CCPU@
#CPUS+=PDP1@
CPUS+=ADSP2100@
CPUS+=ADSP2105@
CPUS+=MIPS@
#CPUS+=SC61860@
#CPUS+=ARM@

# uncomment the following lines to include a sound core
SOUNDS+=CUSTOM@
SOUNDS+=SAMPLES@
SOUNDS+=DAC@
SOUNDS+=AY8910@
SOUNDS+=YM2203@
# enable only one of the following two
#SOUNDS+=YM2151@
SOUNDS+=YM2151_ALT@
SOUNDS+=YM2608@
SOUNDS+=YM2610@
SOUNDS+=YM2610B@
SOUNDS+=YM2612@
SOUNDS+=YM3438@
SOUNDS+=YM2413@
SOUNDS+=YM3812@
SOUNDS+=YMZ280B@
SOUNDS+=YM3526@
SOUNDS+=Y8950@
SOUNDS+=SN76477@
SOUNDS+=SN76496@
SOUNDS+=POKEY@
#SOUNDS+=TIA@
SOUNDS+=NES@
SOUNDS+=ASTROCADE@
SOUNDS+=NAMCO@
SOUNDS+=TMS36XX@
SOUNDS+=TMS5220@
SOUNDS+=VLM5030@
SOUNDS+=ADPCM@
SOUNDS+=OKIM6295@
SOUNDS+=MSM5205@
SOUNDS+=UPD7759@
SOUNDS+=HC55516@
SOUNDS+=K005289@
SOUNDS+=K007232@
SOUNDS+=K051649@
SOUNDS+=K053260@
SOUNDS+=SEGAPCM@
SOUNDS+=RF5C68@
SOUNDS+=CEM3394@
SOUNDS+=C140@
SOUNDS+=QSOUND@
#SOUNDS+=SPEAKER@
#SOUNDS+=WAVE@

DRVLIBS = \
	$(OBJ)/pacman.a $(OBJ)/nichibut.a \
	$(OBJ)/phoenix.a $(OBJ)/namco.a $(OBJ)/univers.a $(OBJ)/nintendo.a \
	$(OBJ)/midw8080.a $(OBJ)/meadows.a $(OBJ)/midway.a \
	$(OBJ)/irem.a $(OBJ)/gottlieb.a $(OBJ)/taito.a $(OBJ)/toaplan.a $(OBJ)/cave.a \
	$(OBJ)/kyugo.a $(OBJ)/williams.a $(OBJ)/gremlin.a $(OBJ)/vicdual.a \
	$(OBJ)/capcom.a $(OBJ)/capbowl.a $(OBJ)/leland.a \
	$(OBJ)/sega.a $(OBJ)/dataeast.a $(OBJ)/tehkan.a $(OBJ)/konami.a \
	$(OBJ)/exidy.a $(OBJ)/atari.a $(OBJ)/snk.a $(OBJ)/alpha.a $(OBJ)/technos.a \
	$(OBJ)/berzerk.a $(OBJ)/gameplan.a $(OBJ)/stratvox.a $(OBJ)/zaccaria.a \
	$(OBJ)/upl.a $(OBJ)/tms.a $(OBJ)/cinemar.a $(OBJ)/cinemav.a $(OBJ)/thepit.a \
	$(OBJ)/valadon.a $(OBJ)/seibu.a $(OBJ)/tad.a $(OBJ)/jaleco.a \
	$(OBJ)/vsystem.a $(OBJ)/psikyo.a $(OBJ)/orca.a $(OBJ)/gaelco.a \
	$(OBJ)/kaneko.a $(OBJ)/seta.a $(OBJ)/atlus.a $(OBJ)/sun.a \
	$(OBJ)/dooyong.a $(OBJ)/nmk.a $(OBJ)/other.a $(OBJ)/neogeo.a \

$(OBJ)/pacman.a: \
	$(OBJ)/drivers/pacman.o \
	$(OBJ)/machine/pacplus.o \
	$(OBJ)/machine/theglob.o \
	$(OBJ)/drivers/jrpacman.o $(OBJ)/vidhrdw/jrpacman.o \
	$(OBJ)/vidhrdw/pengo.o $(OBJ)/drivers/pengo.o \

$(OBJ)/nichibut.a: \
	$(OBJ)/vidhrdw/cclimber.o $(OBJ)/sndhrdw/cclimber.o $(OBJ)/drivers/cclimber.o \
	$(OBJ)/drivers/yamato.o \
	$(OBJ)/vidhrdw/seicross.o $(OBJ)/sndhrdw/wiping.o $(OBJ)/drivers/seicross.o \
	$(OBJ)/vidhrdw/wiping.o $(OBJ)/drivers/wiping.o \
	$(OBJ)/vidhrdw/cop01.o $(OBJ)/drivers/cop01.o \
	$(OBJ)/vidhrdw/terracre.o $(OBJ)/drivers/terracre.o \
	$(OBJ)/vidhrdw/galivan.o $(OBJ)/drivers/galivan.o \
	$(OBJ)/vidhrdw/armedf.o $(OBJ)/drivers/armedf.o \

$(OBJ)/phoenix.a: \
	$(OBJ)/drivers/safarir.o \
	$(OBJ)/vidhrdw/phoenix.o $(OBJ)/sndhrdw/phoenix.o $(OBJ)/drivers/phoenix.o \
	$(OBJ)/sndhrdw/pleiads.o \
	$(OBJ)/vidhrdw/naughtyb.o $(OBJ)/drivers/naughtyb.o \

$(OBJ)/namco.a: \
	$(OBJ)/machine/geebee.o $(OBJ)/vidhrdw/geebee.o $(OBJ)/sndhrdw/geebee.o $(OBJ)/drivers/geebee.o \
	$(OBJ)/vidhrdw/warpwarp.o $(OBJ)/sndhrdw/warpwarp.o $(OBJ)/drivers/warpwarp.o \
	$(OBJ)/vidhrdw/tankbatt.o $(OBJ)/drivers/tankbatt.o \
	$(OBJ)/vidhrdw/galaxian.o $(OBJ)/sndhrdw/galaxian.o $(OBJ)/drivers/galaxian.o \
	$(OBJ)/vidhrdw/rallyx.o $(OBJ)/drivers/rallyx.o \
	$(OBJ)/drivers/locomotn.o \
	$(OBJ)/machine/bosco.o $(OBJ)/sndhrdw/bosco.o $(OBJ)/vidhrdw/bosco.o $(OBJ)/drivers/bosco.o \
	$(OBJ)/machine/galaga.o $(OBJ)/vidhrdw/galaga.o $(OBJ)/drivers/galaga.o \
	$(OBJ)/machine/digdug.o $(OBJ)/vidhrdw/digdug.o $(OBJ)/drivers/digdug.o \
	$(OBJ)/vidhrdw/xevious.o $(OBJ)/machine/xevious.o $(OBJ)/drivers/xevious.o \
	$(OBJ)/machine/superpac.o $(OBJ)/vidhrdw/superpac.o $(OBJ)/drivers/superpac.o \
	$(OBJ)/machine/phozon.o $(OBJ)/vidhrdw/phozon.o $(OBJ)/drivers/phozon.o \
	$(OBJ)/machine/mappy.o $(OBJ)/vidhrdw/mappy.o $(OBJ)/drivers/mappy.o \
	$(OBJ)/machine/grobda.o $(OBJ)/vidhrdw/grobda.o $(OBJ)/drivers/grobda.o \
	$(OBJ)/machine/gaplus.o $(OBJ)/vidhrdw/gaplus.o $(OBJ)/drivers/gaplus.o \
	$(OBJ)/machine/toypop.o $(OBJ)/vidhrdw/toypop.o $(OBJ)/drivers/toypop.o \
	$(OBJ)/machine/polepos.o $(OBJ)/vidhrdw/polepos.o $(OBJ)/sndhrdw/polepos.o $(OBJ)/drivers/polepos.o \
	$(OBJ)/vidhrdw/pacland.o $(OBJ)/drivers/pacland.o \
	$(OBJ)/vidhrdw/skykid.o $(OBJ)/drivers/skykid.o \
	$(OBJ)/vidhrdw/baraduke.o $(OBJ)/drivers/baraduke.o \
	$(OBJ)/vidhrdw/namcos86.o $(OBJ)/drivers/namcos86.o \
	$(OBJ)/machine/namcos1.o $(OBJ)/vidhrdw/namcos1.o $(OBJ)/drivers/namcos1.o \
	$(OBJ)/machine/namcos2.o $(OBJ)/vidhrdw/namcos2.o $(OBJ)/drivers/namcos2.o \

$(OBJ)/univers.a: \
	$(OBJ)/vidhrdw/cosmic.o $(OBJ)/drivers/cosmic.o \
	$(OBJ)/vidhrdw/cheekyms.o $(OBJ)/drivers/cheekyms.o \
	$(OBJ)/vidhrdw/ladybug.o $(OBJ)/drivers/ladybug.o \
	$(OBJ)/vidhrdw/mrdo.o $(OBJ)/drivers/mrdo.o \
	$(OBJ)/machine/docastle.o $(OBJ)/vidhrdw/docastle.o $(OBJ)/drivers/docastle.o \

$(OBJ)/nintendo.a: \
	$(OBJ)/vidhrdw/dkong.o $(OBJ)/sndhrdw/dkong.o $(OBJ)/drivers/dkong.o \
	$(OBJ)/vidhrdw/mario.o $(OBJ)/sndhrdw/mario.o $(OBJ)/drivers/mario.o \
	$(OBJ)/vidhrdw/popeye.o $(OBJ)/drivers/popeye.o \
	$(OBJ)/vidhrdw/punchout.o $(OBJ)/drivers/punchout.o \

$(OBJ)/midw8080.a: \
	$(OBJ)/machine/8080bw.o $(OBJ)/machine/74123.o \
	$(OBJ)/vidhrdw/8080bw.o $(OBJ)/sndhrdw/8080bw.o $(OBJ)/drivers/8080bw.o \
	$(OBJ)/vidhrdw/m79amb.o $(OBJ)/drivers/m79amb.o \
	$(OBJ)/sndhrdw/z80bw.o $(OBJ)/drivers/z80bw.o \

$(OBJ)/meadows.a: \
	$(OBJ)/drivers/lazercmd.o $(OBJ)/vidhrdw/lazercmd.o \
	$(OBJ)/drivers/meadows.o $(OBJ)/sndhrdw/meadows.o $(OBJ)/vidhrdw/meadows.o \

$(OBJ)/midway.a: \
	$(OBJ)/machine/astrocde.o $(OBJ)/vidhrdw/astrocde.o \
	$(OBJ)/sndhrdw/astrocde.o $(OBJ)/sndhrdw/gorf.o $(OBJ)/drivers/astrocde.o \
	$(OBJ)/machine/mcr.o $(OBJ)/sndhrdw/mcr.o \
	$(OBJ)/vidhrdw/mcr12.o $(OBJ)/vidhrdw/mcr3.o \
	$(OBJ)/drivers/mcr1.o $(OBJ)/drivers/mcr2.o $(OBJ)/drivers/mcr3.o \
	$(OBJ)/vidhrdw/mcr68.o $(OBJ)/drivers/mcr68.o \
	$(OBJ)/vidhrdw/balsente.o $(OBJ)/drivers/balsente.o \

$(OBJ)/irem.a: \
	$(OBJ)/vidhrdw/skychut.o $(OBJ)/drivers/skychut.o \
	$(OBJ)/drivers/olibochu.o \
	$(OBJ)/sndhrdw/irem.o \
	$(OBJ)/vidhrdw/mpatrol.o $(OBJ)/drivers/mpatrol.o \
	$(OBJ)/vidhrdw/troangel.o $(OBJ)/drivers/troangel.o \
	$(OBJ)/vidhrdw/yard.o $(OBJ)/drivers/yard.o \
	$(OBJ)/vidhrdw/travrusa.o $(OBJ)/drivers/travrusa.o \
	$(OBJ)/vidhrdw/m62.o $(OBJ)/drivers/m62.o \
	$(OBJ)/vidhrdw/vigilant.o $(OBJ)/drivers/vigilant.o \
	$(OBJ)/vidhrdw/m72.o $(OBJ)/sndhrdw/m72.o $(OBJ)/drivers/m72.o \
	$(OBJ)/vidhrdw/shisen.o $(OBJ)/drivers/shisen.o \
	$(OBJ)/vidhrdw/m92.o $(OBJ)/drivers/m92.o \
	$(OBJ)/drivers/m97.o \
	$(OBJ)/vidhrdw/m107.o $(OBJ)/drivers/m107.o \

$(OBJ)/gottlieb.a: \
	$(OBJ)/vidhrdw/gottlieb.o $(OBJ)/sndhrdw/gottlieb.o $(OBJ)/drivers/gottlieb.o \

$(OBJ)/taito.a: \
	$(OBJ)/vidhrdw/crbaloon.o $(OBJ)/drivers/crbaloon.o \
	$(OBJ)/machine/qix.o $(OBJ)/vidhrdw/qix.o $(OBJ)/drivers/qix.o \
	$(OBJ)/machine/taitosj.o $(OBJ)/vidhrdw/taitosj.o $(OBJ)/drivers/taitosj.o \
	$(OBJ)/vidhrdw/bking2.o $(OBJ)/drivers/bking2.o \
	$(OBJ)/vidhrdw/gsword.o $(OBJ)/drivers/gsword.o $(OBJ)/machine/tait8741.o \
	$(OBJ)/vidhrdw/retofinv.o $(OBJ)/drivers/retofinv.o \
	$(OBJ)/vidhrdw/tsamurai.o $(OBJ)/drivers/tsamurai.o \
	$(OBJ)/machine/flstory.o $(OBJ)/vidhrdw/flstory.o $(OBJ)/drivers/flstory.o \
	$(OBJ)/vidhrdw/gladiatr.o $(OBJ)/drivers/gladiatr.o \
	$(OBJ)/machine/lsasquad.o $(OBJ)/vidhrdw/lsasquad.o $(OBJ)/drivers/lsasquad.o \
	$(OBJ)/machine/bublbobl.o $(OBJ)/vidhrdw/bublbobl.o $(OBJ)/drivers/bublbobl.o \
	$(OBJ)/machine/mexico86.o $(OBJ)/vidhrdw/mexico86.o $(OBJ)/drivers/mexico86.o \
	$(OBJ)/vidhrdw/rastan.o $(OBJ)/sndhrdw/rastan.o $(OBJ)/drivers/rastan.o \
	$(OBJ)/machine/rainbow.o $(OBJ)/drivers/rainbow.o \
	$(OBJ)/machine/arkanoid.o $(OBJ)/vidhrdw/arkanoid.o $(OBJ)/drivers/arkanoid.o \
	$(OBJ)/vidhrdw/superqix.o $(OBJ)/drivers/superqix.o \
	$(OBJ)/vidhrdw/superman.o $(OBJ)/drivers/superman.o $(OBJ)/machine/cchip.o \
	$(OBJ)/vidhrdw/minivadr.o $(OBJ)/drivers/minivadr.o \
	$(OBJ)/machine/tnzs.o $(OBJ)/vidhrdw/tnzs.o $(OBJ)/drivers/tnzs.o \
	$(OBJ)/machine/lkage.o $(OBJ)/drivers/lkage.o $(OBJ)/vidhrdw/lkage.o \
	$(OBJ)/vidhrdw/taito_l.o $(OBJ)/drivers/taito_l.o \
	$(OBJ)/vidhrdw/taito_b.o $(OBJ)/drivers/taito_b.o \
	$(OBJ)/vidhrdw/taitoic.o \
	$(OBJ)/vidhrdw/taito_f2.o $(OBJ)/drivers/taito_f2.o \

$(OBJ)/toaplan.a: \
	$(OBJ)/machine/slapfght.o $(OBJ)/vidhrdw/slapfght.o $(OBJ)/drivers/slapfght.o \
	$(OBJ)/machine/twincobr.o $(OBJ)/vidhrdw/twincobr.o \
	$(OBJ)/drivers/twincobr.o $(OBJ)/drivers/wardner.o \
	$(OBJ)/machine/toaplan1.o $(OBJ)/vidhrdw/toaplan1.o $(OBJ)/drivers/toaplan1.o \
	$(OBJ)/vidhrdw/snowbros.o $(OBJ)/drivers/snowbros.o \
	$(OBJ)/vidhrdw/toaplan2.o $(OBJ)/drivers/toaplan2.o \

$(OBJ)/cave.a: \
	$(OBJ)/vidhrdw/cave.o $(OBJ)/drivers/cave.o \

$(OBJ)/kyugo.a: \
	$(OBJ)/drivers/kyugo.o $(OBJ)/vidhrdw/kyugo.o \

$(OBJ)/williams.a: \
	$(OBJ)/machine/williams.o $(OBJ)/vidhrdw/williams.o $(OBJ)/sndhrdw/williams.o $(OBJ)/drivers/williams.o \

$(OBJ)/capcom.a: \
	$(OBJ)/vidhrdw/vulgus.o $(OBJ)/drivers/vulgus.o \
	$(OBJ)/vidhrdw/sonson.o $(OBJ)/drivers/sonson.o \
	$(OBJ)/vidhrdw/higemaru.o $(OBJ)/drivers/higemaru.o \
	$(OBJ)/vidhrdw/1942.o $(OBJ)/drivers/1942.o \
	$(OBJ)/vidhrdw/exedexes.o $(OBJ)/drivers/exedexes.o \
	$(OBJ)/vidhrdw/commando.o $(OBJ)/drivers/commando.o \
	$(OBJ)/vidhrdw/gng.o $(OBJ)/drivers/gng.o \
	$(OBJ)/vidhrdw/gunsmoke.o $(OBJ)/drivers/gunsmoke.o \
	$(OBJ)/vidhrdw/srumbler.o $(OBJ)/drivers/srumbler.o \
	$(OBJ)/machine/lwings.o $(OBJ)/vidhrdw/lwings.o $(OBJ)/drivers/lwings.o \
	$(OBJ)/vidhrdw/sidearms.o $(OBJ)/drivers/sidearms.o \
	$(OBJ)/vidhrdw/bionicc.o $(OBJ)/drivers/bionicc.o \
	$(OBJ)/vidhrdw/1943.o $(OBJ)/drivers/1943.o \
	$(OBJ)/vidhrdw/blktiger.o $(OBJ)/drivers/blktiger.o \
	$(OBJ)/vidhrdw/tigeroad.o $(OBJ)/drivers/tigeroad.o \
	$(OBJ)/vidhrdw/lastduel.o $(OBJ)/drivers/lastduel.o \
	$(OBJ)/vidhrdw/sf1.o $(OBJ)/drivers/sf1.o \
	$(OBJ)/machine/kabuki.o \
	$(OBJ)/vidhrdw/mitchell.o $(OBJ)/drivers/mitchell.o \
	$(OBJ)/vidhrdw/cbasebal.o $(OBJ)/drivers/cbasebal.o \
	$(OBJ)/vidhrdw/cps1.o $(OBJ)/drivers/cps1.o \
	$(OBJ)/drivers/zn.o \

$(OBJ)/capbowl.a: \
	$(OBJ)/machine/capbowl.o $(OBJ)/vidhrdw/capbowl.o $(OBJ)/vidhrdw/tms34061.o $(OBJ)/drivers/capbowl.o \

$(OBJ)/gremlin.a: \
	$(OBJ)/vidhrdw/blockade.o $(OBJ)/drivers/blockade.o \

$(OBJ)/vicdual.a: \
	$(OBJ)/vidhrdw/vicdual.o $(OBJ)/drivers/vicdual.o \
	$(OBJ)/sndhrdw/carnival.o $(OBJ)/sndhrdw/depthch.o $(OBJ)/sndhrdw/invinco.o $(OBJ)/sndhrdw/pulsar.o \

$(OBJ)/sega.a: \
	$(OBJ)/machine/segacrpt.o \
	$(OBJ)/vidhrdw/sega.o $(OBJ)/sndhrdw/sega.o $(OBJ)/machine/sega.o $(OBJ)/drivers/sega.o \
	$(OBJ)/vidhrdw/segar.o $(OBJ)/sndhrdw/segar.o $(OBJ)/machine/segar.o $(OBJ)/drivers/segar.o \
	$(OBJ)/vidhrdw/zaxxon.o $(OBJ)/sndhrdw/zaxxon.o $(OBJ)/drivers/zaxxon.o \
	$(OBJ)/drivers/congo.o \
	$(OBJ)/machine/turbo.o $(OBJ)/vidhrdw/turbo.o $(OBJ)/drivers/turbo.o \
	$(OBJ)/drivers/kopunch.o \
	$(OBJ)/vidhrdw/suprloco.o $(OBJ)/drivers/suprloco.o \
	$(OBJ)/vidhrdw/appoooh.o $(OBJ)/drivers/appoooh.o \
	$(OBJ)/vidhrdw/bankp.o $(OBJ)/drivers/bankp.o \
	$(OBJ)/vidhrdw/dotrikun.o $(OBJ)/drivers/dotrikun.o \
	$(OBJ)/vidhrdw/system1.o $(OBJ)/drivers/system1.o \
	$(OBJ)/machine/system16.o $(OBJ)/vidhrdw/system16.o $(OBJ)/sndhrdw/system16.o $(OBJ)/drivers/system16.o \

$(OBJ)/dataeast.a: \
	$(OBJ)/machine/btime.o $(OBJ)/vidhrdw/btime.o $(OBJ)/drivers/btime.o \
	$(OBJ)/vidhrdw/astrof.o $(OBJ)/sndhrdw/astrof.o $(OBJ)/drivers/astrof.o \
	$(OBJ)/vidhrdw/kchamp.o $(OBJ)/drivers/kchamp.o \
	$(OBJ)/vidhrdw/firetrap.o $(OBJ)/drivers/firetrap.o \
	$(OBJ)/vidhrdw/brkthru.o $(OBJ)/drivers/brkthru.o \
	$(OBJ)/vidhrdw/shootout.o $(OBJ)/drivers/shootout.o \
	$(OBJ)/vidhrdw/sidepckt.o $(OBJ)/drivers/sidepckt.o \
	$(OBJ)/vidhrdw/exprraid.o $(OBJ)/drivers/exprraid.o \
	$(OBJ)/vidhrdw/pcktgal.o $(OBJ)/drivers/pcktgal.o \
	$(OBJ)/vidhrdw/battlera.o $(OBJ)/drivers/battlera.o \
	$(OBJ)/vidhrdw/actfancr.o $(OBJ)/drivers/actfancr.o \
	$(OBJ)/vidhrdw/dec8.o $(OBJ)/drivers/dec8.o \
	$(OBJ)/vidhrdw/karnov.o $(OBJ)/drivers/karnov.o \
	$(OBJ)/machine/dec0.o $(OBJ)/vidhrdw/dec0.o $(OBJ)/drivers/dec0.o \
	$(OBJ)/vidhrdw/stadhero.o $(OBJ)/drivers/stadhero.o \
	$(OBJ)/vidhrdw/madmotor.o $(OBJ)/drivers/madmotor.o \
	$(OBJ)/vidhrdw/vaportra.o $(OBJ)/drivers/vaportra.o \
	$(OBJ)/vidhrdw/cbuster.o $(OBJ)/drivers/cbuster.o \
	$(OBJ)/vidhrdw/darkseal.o $(OBJ)/drivers/darkseal.o \
	$(OBJ)/vidhrdw/supbtime.o $(OBJ)/drivers/supbtime.o \
	$(OBJ)/vidhrdw/cninja.o $(OBJ)/drivers/cninja.o \
	$(OBJ)/vidhrdw/tumblep.o $(OBJ)/drivers/tumblep.o \
	$(OBJ)/vidhrdw/funkyjet.o $(OBJ)/drivers/funkyjet.o \

$(OBJ)/tehkan.a: \
	$(OBJ)/sndhrdw/senjyo.o $(OBJ)/vidhrdw/senjyo.o $(OBJ)/drivers/senjyo.o \
	$(OBJ)/vidhrdw/bombjack.o $(OBJ)/drivers/bombjack.o \
	$(OBJ)/vidhrdw/pbaction.o $(OBJ)/drivers/pbaction.o \
	$(OBJ)/vidhrdw/tehkanwc.o $(OBJ)/drivers/tehkanwc.o \
	$(OBJ)/vidhrdw/solomon.o $(OBJ)/drivers/solomon.o \
	$(OBJ)/vidhrdw/tecmo.o $(OBJ)/drivers/tecmo.o \
	$(OBJ)/vidhrdw/gaiden.o $(OBJ)/drivers/gaiden.o \
	$(OBJ)/vidhrdw/wc90.o $(OBJ)/drivers/wc90.o \
	$(OBJ)/vidhrdw/wc90b.o $(OBJ)/drivers/wc90b.o \
	$(OBJ)/vidhrdw/tecmo16.o $(OBJ)/drivers/tecmo16.o \

$(OBJ)/konami.a: \
	$(OBJ)/machine/scramble.o $(OBJ)/sndhrdw/scramble.o $(OBJ)/drivers/scramble.o \
	$(OBJ)/vidhrdw/frogger.o $(OBJ)/sndhrdw/frogger.o $(OBJ)/drivers/frogger.o \
	$(OBJ)/drivers/scobra.o \
	$(OBJ)/vidhrdw/amidar.o $(OBJ)/drivers/amidar.o \
	$(OBJ)/vidhrdw/fastfred.o $(OBJ)/drivers/fastfred.o \
	$(OBJ)/sndhrdw/timeplt.o \
	$(OBJ)/vidhrdw/tutankhm.o $(OBJ)/drivers/tutankhm.o \
	$(OBJ)/drivers/junofrst.o \
	$(OBJ)/vidhrdw/pooyan.o $(OBJ)/drivers/pooyan.o \
	$(OBJ)/vidhrdw/timeplt.o $(OBJ)/drivers/timeplt.o \
	$(OBJ)/vidhrdw/megazone.o $(OBJ)/drivers/megazone.o \
	$(OBJ)/vidhrdw/pandoras.o $(OBJ)/drivers/pandoras.o \
	$(OBJ)/sndhrdw/gyruss.o $(OBJ)/vidhrdw/gyruss.o $(OBJ)/drivers/gyruss.o \
	$(OBJ)/machine/konami.o $(OBJ)/vidhrdw/trackfld.o $(OBJ)/sndhrdw/trackfld.o $(OBJ)/drivers/trackfld.o \
	$(OBJ)/vidhrdw/rocnrope.o $(OBJ)/drivers/rocnrope.o \
	$(OBJ)/vidhrdw/circusc.o $(OBJ)/drivers/circusc.o \
	$(OBJ)/vidhrdw/tp84.o $(OBJ)/drivers/tp84.o \
	$(OBJ)/vidhrdw/hyperspt.o $(OBJ)/drivers/hyperspt.o \
	$(OBJ)/vidhrdw/sbasketb.o $(OBJ)/drivers/sbasketb.o \
	$(OBJ)/vidhrdw/mikie.o $(OBJ)/drivers/mikie.o \
	$(OBJ)/vidhrdw/yiear.o $(OBJ)/drivers/yiear.o \
	$(OBJ)/vidhrdw/shaolins.o $(OBJ)/drivers/shaolins.o \
	$(OBJ)/vidhrdw/pingpong.o $(OBJ)/drivers/pingpong.o \
	$(OBJ)/vidhrdw/gberet.o $(OBJ)/drivers/gberet.o \
	$(OBJ)/vidhrdw/jailbrek.o $(OBJ)/drivers/jailbrek.o \
	$(OBJ)/vidhrdw/finalizr.o $(OBJ)/drivers/finalizr.o \
	$(OBJ)/vidhrdw/ironhors.o $(OBJ)/drivers/ironhors.o \
	$(OBJ)/machine/jackal.o $(OBJ)/vidhrdw/jackal.o $(OBJ)/drivers/jackal.o \
	$(OBJ)/machine/ddrible.o $(OBJ)/vidhrdw/ddrible.o $(OBJ)/drivers/ddrible.o \
	$(OBJ)/vidhrdw/contra.o $(OBJ)/drivers/contra.o \
	$(OBJ)/vidhrdw/combatsc.o $(OBJ)/drivers/combatsc.o \
	$(OBJ)/vidhrdw/hcastle.o $(OBJ)/drivers/hcastle.o \
	$(OBJ)/vidhrdw/nemesis.o $(OBJ)/drivers/nemesis.o \
	$(OBJ)/vidhrdw/konamiic.o \
	$(OBJ)/vidhrdw/rockrage.o $(OBJ)/drivers/rockrage.o \
	$(OBJ)/vidhrdw/flkatck.o $(OBJ)/drivers/flkatck.o \
	$(OBJ)/vidhrdw/fastlane.o $(OBJ)/drivers/fastlane.o \
	$(OBJ)/vidhrdw/labyrunr.o $(OBJ)/drivers/labyrunr.o \
	$(OBJ)/vidhrdw/battlnts.o $(OBJ)/drivers/battlnts.o \
	$(OBJ)/vidhrdw/bladestl.o $(OBJ)/drivers/bladestl.o \
	$(OBJ)/machine/ajax.o $(OBJ)/vidhrdw/ajax.o $(OBJ)/drivers/ajax.o \
	$(OBJ)/vidhrdw/thunderx.o $(OBJ)/drivers/thunderx.o \
	$(OBJ)/vidhrdw/mainevt.o $(OBJ)/drivers/mainevt.o \
	$(OBJ)/vidhrdw/88games.o $(OBJ)/drivers/88games.o \
	$(OBJ)/vidhrdw/gbusters.o $(OBJ)/drivers/gbusters.o \
	$(OBJ)/vidhrdw/crimfght.o $(OBJ)/drivers/crimfght.o \
	$(OBJ)/vidhrdw/spy.o $(OBJ)/drivers/spy.o \
	$(OBJ)/vidhrdw/bottom9.o $(OBJ)/drivers/bottom9.o \
	$(OBJ)/vidhrdw/blockhl.o $(OBJ)/drivers/blockhl.o \
	$(OBJ)/vidhrdw/aliens.o $(OBJ)/drivers/aliens.o \
	$(OBJ)/vidhrdw/surpratk.o $(OBJ)/drivers/surpratk.o \
	$(OBJ)/vidhrdw/parodius.o $(OBJ)/drivers/parodius.o \
	$(OBJ)/vidhrdw/rollerg.o $(OBJ)/drivers/rollerg.o \
	$(OBJ)/vidhrdw/xexex.o $(OBJ)/drivers/xexex.o \
	$(OBJ)/machine/simpsons.o $(OBJ)/vidhrdw/simpsons.o $(OBJ)/drivers/simpsons.o \
	$(OBJ)/vidhrdw/vendetta.o $(OBJ)/drivers/vendetta.o \
	$(OBJ)/vidhrdw/twin16.o $(OBJ)/drivers/twin16.o \
	$(OBJ)/vidhrdw/gradius3.o $(OBJ)/drivers/gradius3.o \
	$(OBJ)/vidhrdw/tmnt.o $(OBJ)/drivers/tmnt.o \
	$(OBJ)/vidhrdw/xmen.o $(OBJ)/drivers/xmen.o \
	$(OBJ)/vidhrdw/wecleman.o $(OBJ)/drivers/wecleman.o \
	$(OBJ)/vidhrdw/chqflag.o $(OBJ)/drivers/chqflag.o \
	$(OBJ)/vidhrdw/ultraman.o $(OBJ)/drivers/ultraman.o \

$(OBJ)/exidy.a: \
	$(OBJ)/vidhrdw/exidy.o $(OBJ)/sndhrdw/exidy.o $(OBJ)/drivers/exidy.o \
	$(OBJ)/sndhrdw/targ.o \
	$(OBJ)/vidhrdw/circus.o $(OBJ)/drivers/circus.o \
	$(OBJ)/vidhrdw/starfire.o $(OBJ)/drivers/starfire.o \
	$(OBJ)/vidhrdw/victory.o $(OBJ)/drivers/victory.o \
	$(OBJ)/sndhrdw/exidy440.o $(OBJ)/vidhrdw/exidy440.o $(OBJ)/drivers/exidy440.o \

$(OBJ)/atari.a: \
	$(OBJ)/machine/atari_vg.o \
	$(OBJ)/machine/asteroid.o $(OBJ)/sndhrdw/asteroid.o \
	$(OBJ)/vidhrdw/llander.o $(OBJ)/sndhrdw/llander.o $(OBJ)/drivers/asteroid.o \
	$(OBJ)/drivers/bwidow.o \
	$(OBJ)/sndhrdw/bzone.o	$(OBJ)/drivers/bzone.o \
	$(OBJ)/sndhrdw/redbaron.o \
	$(OBJ)/drivers/tempest.o \
	$(OBJ)/machine/starwars.o $(OBJ)/machine/swmathbx.o \
	$(OBJ)/drivers/starwars.o $(OBJ)/sndhrdw/starwars.o \
	$(OBJ)/machine/mhavoc.o $(OBJ)/drivers/mhavoc.o \
	$(OBJ)/machine/quantum.o $(OBJ)/drivers/quantum.o \
	$(OBJ)/machine/atarifb.o $(OBJ)/vidhrdw/atarifb.o $(OBJ)/drivers/atarifb.o \
	$(OBJ)/machine/sprint2.o $(OBJ)/vidhrdw/sprint2.o $(OBJ)/drivers/sprint2.o \
	$(OBJ)/machine/sbrkout.o $(OBJ)/vidhrdw/sbrkout.o $(OBJ)/drivers/sbrkout.o \
	$(OBJ)/machine/dominos.o $(OBJ)/vidhrdw/dominos.o $(OBJ)/drivers/dominos.o \
	$(OBJ)/vidhrdw/nitedrvr.o $(OBJ)/machine/nitedrvr.o $(OBJ)/drivers/nitedrvr.o \
	$(OBJ)/vidhrdw/bsktball.o $(OBJ)/machine/bsktball.o $(OBJ)/drivers/bsktball.o \
	$(OBJ)/vidhrdw/copsnrob.o $(OBJ)/machine/copsnrob.o $(OBJ)/drivers/copsnrob.o \
	$(OBJ)/machine/avalnche.o $(OBJ)/vidhrdw/avalnche.o $(OBJ)/drivers/avalnche.o \
	$(OBJ)/machine/subs.o $(OBJ)/vidhrdw/subs.o $(OBJ)/drivers/subs.o \
	$(OBJ)/vidhrdw/canyon.o $(OBJ)/drivers/canyon.o \
	$(OBJ)/vidhrdw/skydiver.o $(OBJ)/drivers/skydiver.o \
	$(OBJ)/vidhrdw/warlord.o $(OBJ)/drivers/warlord.o \
	$(OBJ)/vidhrdw/centiped.o $(OBJ)/drivers/centiped.o \
	$(OBJ)/vidhrdw/milliped.o $(OBJ)/drivers/milliped.o \
	$(OBJ)/vidhrdw/qwakprot.o $(OBJ)/drivers/qwakprot.o \
	$(OBJ)/vidhrdw/kangaroo.o $(OBJ)/drivers/kangaroo.o \
	$(OBJ)/vidhrdw/arabian.o $(OBJ)/drivers/arabian.o \
	$(OBJ)/machine/missile.o $(OBJ)/vidhrdw/missile.o $(OBJ)/drivers/missile.o \
	$(OBJ)/machine/foodf.o $(OBJ)/vidhrdw/foodf.o $(OBJ)/drivers/foodf.o \
	$(OBJ)/vidhrdw/liberatr.o $(OBJ)/drivers/liberatr.o \
	$(OBJ)/vidhrdw/ccastles.o $(OBJ)/drivers/ccastles.o \
	$(OBJ)/vidhrdw/cloak.o $(OBJ)/drivers/cloak.o \
	$(OBJ)/vidhrdw/cloud9.o $(OBJ)/drivers/cloud9.o \
	$(OBJ)/machine/jedi.o $(OBJ)/vidhrdw/jedi.o $(OBJ)/sndhrdw/jedi.o $(OBJ)/drivers/jedi.o \
	$(OBJ)/machine/atarigen.o $(OBJ)/sndhrdw/atarijsa.o \
	$(OBJ)/machine/slapstic.o \
	$(OBJ)/vidhrdw/atarisy1.o $(OBJ)/drivers/atarisy1.o \
	$(OBJ)/vidhrdw/atarisy2.o $(OBJ)/drivers/atarisy2.o \
	$(OBJ)/vidhrdw/gauntlet.o $(OBJ)/drivers/gauntlet.o \
	$(OBJ)/vidhrdw/atetris.o $(OBJ)/drivers/atetris.o \
	$(OBJ)/vidhrdw/toobin.o $(OBJ)/drivers/toobin.o \
	$(OBJ)/vidhrdw/vindictr.o $(OBJ)/drivers/vindictr.o \
	$(OBJ)/vidhrdw/klax.o $(OBJ)/drivers/klax.o \
	$(OBJ)/vidhrdw/blstroid.o $(OBJ)/drivers/blstroid.o \
	$(OBJ)/vidhrdw/xybots.o $(OBJ)/drivers/xybots.o \
	$(OBJ)/vidhrdw/eprom.o $(OBJ)/drivers/eprom.o \
	$(OBJ)/vidhrdw/skullxbo.o $(OBJ)/drivers/skullxbo.o \
	$(OBJ)/vidhrdw/badlands.o $(OBJ)/drivers/badlands.o \
	$(OBJ)/vidhrdw/cyberbal.o $(OBJ)/drivers/cyberbal.o \
	$(OBJ)/vidhrdw/rampart.o $(OBJ)/drivers/rampart.o \
	$(OBJ)/vidhrdw/shuuz.o $(OBJ)/drivers/shuuz.o \
	$(OBJ)/vidhrdw/hydra.o $(OBJ)/drivers/hydra.o \
	$(OBJ)/vidhrdw/thunderj.o $(OBJ)/drivers/thunderj.o \
	$(OBJ)/vidhrdw/batman.o $(OBJ)/drivers/batman.o \
	$(OBJ)/vidhrdw/relief.o $(OBJ)/drivers/relief.o \
	$(OBJ)/vidhrdw/offtwall.o $(OBJ)/drivers/offtwall.o \
	$(OBJ)/vidhrdw/arcadecl.o $(OBJ)/drivers/arcadecl.o \

$(OBJ)/snk.a: \
	$(OBJ)/vidhrdw/rockola.o $(OBJ)/sndhrdw/rockola.o $(OBJ)/drivers/rockola.o \
	$(OBJ)/vidhrdw/lasso.o $(OBJ)/drivers/lasso.o \
	$(OBJ)/drivers/munchmo.o $(OBJ)/vidhrdw/munchmo.o \
	$(OBJ)/vidhrdw/marvins.o $(OBJ)/drivers/marvins.o \
	$(OBJ)/drivers/hal21.o \
	$(OBJ)/vidhrdw/snk.o $(OBJ)/drivers/snk.o \
	$(OBJ)/vidhrdw/snk68.o $(OBJ)/drivers/snk68.o \
	$(OBJ)/vidhrdw/prehisle.o $(OBJ)/drivers/prehisle.o \

$(OBJ)/alpha.a: \
	$(OBJ)/vidhrdw/alpha68k.o $(OBJ)/drivers/alpha68k.o \
	$(OBJ)/vidhrdw/champbas.o $(OBJ)/drivers/champbas.o \
	$(OBJ)/machine/exctsccr.o $(OBJ)/vidhrdw/exctsccr.o $(OBJ)/drivers/exctsccr.o \

$(OBJ)/technos.a: \
	$(OBJ)/drivers/scregg.o \
	$(OBJ)/vidhrdw/tagteam.o $(OBJ)/drivers/tagteam.o \
	$(OBJ)/vidhrdw/ssozumo.o $(OBJ)/drivers/ssozumo.o \
	$(OBJ)/vidhrdw/mystston.o $(OBJ)/drivers/mystston.o \
	$(OBJ)/vidhrdw/bogeyman.o $(OBJ)/drivers/bogeyman.o \
	$(OBJ)/vidhrdw/matmania.o $(OBJ)/drivers/matmania.o $(OBJ)/machine/maniach.o \
	$(OBJ)/vidhrdw/renegade.o $(OBJ)/drivers/renegade.o \
	$(OBJ)/vidhrdw/xain.o $(OBJ)/drivers/xain.o \
	$(OBJ)/vidhrdw/battlane.o $(OBJ)/drivers/battlane.o \
	$(OBJ)/vidhrdw/ddragon.o $(OBJ)/drivers/ddragon.o \
	$(OBJ)/vidhrdw/ddragon3.o $(OBJ)/drivers/ddragon3.o \
	$(OBJ)/vidhrdw/blockout.o $(OBJ)/drivers/blockout.o \

$(OBJ)/berzerk.a: \
	$(OBJ)/machine/berzerk.o $(OBJ)/vidhrdw/berzerk.o $(OBJ)/sndhrdw/berzerk.o $(OBJ)/drivers/berzerk.o \

$(OBJ)/gameplan.a: \
	$(OBJ)/vidhrdw/gameplan.o $(OBJ)/drivers/gameplan.o \

$(OBJ)/stratvox.a: \
	$(OBJ)/vidhrdw/route16.o $(OBJ)/drivers/route16.o \
	$(OBJ)/vidhrdw/ttmahjng.o $(OBJ)/drivers/ttmahjng.o \

$(OBJ)/zaccaria.a: \
	$(OBJ)/vidhrdw/zaccaria.o $(OBJ)/drivers/zaccaria.o \

$(OBJ)/upl.a: \
	$(OBJ)/vidhrdw/nova2001.o $(OBJ)/drivers/nova2001.o \
	$(OBJ)/vidhrdw/pkunwar.o $(OBJ)/drivers/pkunwar.o \
	$(OBJ)/vidhrdw/ninjakd2.o $(OBJ)/drivers/ninjakd2.o \
	$(OBJ)/vidhrdw/mnight.o $(OBJ)/drivers/mnight.o \

$(OBJ)/tms.a: \
	$(OBJ)/machine/exterm.o $(OBJ)/vidhrdw/exterm.o $(OBJ)/drivers/exterm.o \
	$(OBJ)/machine/wmsyunit.o $(OBJ)/vidhrdw/wmsyunit.o $(OBJ)/drivers/wmsyunit.o \
	$(OBJ)/machine/wmstunit.o $(OBJ)/vidhrdw/wmstunit.o $(OBJ)/drivers/wmstunit.o \
	$(OBJ)/machine/wmswolfu.o $(OBJ)/drivers/wmswolfu.o \

$(OBJ)/cinemar.a: \
	$(OBJ)/vidhrdw/jack.o $(OBJ)/drivers/jack.o \

$(OBJ)/cinemav.a: \
	$(OBJ)/sndhrdw/cinemat.o $(OBJ)/drivers/cinemat.o \
	$(OBJ)/machine/cchasm.o $(OBJ)/vidhrdw/cchasm.o $(OBJ)/sndhrdw/cchasm.o $(OBJ)/drivers/cchasm.o \

$(OBJ)/thepit.a: \
	$(OBJ)/vidhrdw/thepit.o $(OBJ)/drivers/thepit.o \

$(OBJ)/valadon.a: \
	$(OBJ)/machine/bagman.o $(OBJ)/vidhrdw/bagman.o $(OBJ)/drivers/bagman.o \

$(OBJ)/seibu.a: \
	$(OBJ)/vidhrdw/wiz.o $(OBJ)/drivers/wiz.o \
	$(OBJ)/machine/stfight.o $(OBJ)/vidhrdw/stfight.o $(OBJ)/drivers/stfight.o \
	$(OBJ)/sndhrdw/seibu.o \
	$(OBJ)/vidhrdw/dynduke.o $(OBJ)/drivers/dynduke.o \
	$(OBJ)/vidhrdw/raiden.o $(OBJ)/drivers/raiden.o \
	$(OBJ)/vidhrdw/dcon.o $(OBJ)/drivers/dcon.o \

$(OBJ)/tad.a: \
	$(OBJ)/vidhrdw/cabal.o $(OBJ)/drivers/cabal.o \
	$(OBJ)/vidhrdw/toki.o $(OBJ)/drivers/toki.o \
	$(OBJ)/vidhrdw/bloodbro.o $(OBJ)/drivers/bloodbro.o \

$(OBJ)/jaleco.a: \
	$(OBJ)/vidhrdw/exerion.o $(OBJ)/drivers/exerion.o \
	$(OBJ)/vidhrdw/aeroboto.o $(OBJ)/drivers/aeroboto.o \
	$(OBJ)/vidhrdw/citycon.o $(OBJ)/drivers/citycon.o \
	$(OBJ)/vidhrdw/pinbo.o $(OBJ)/drivers/pinbo.o \
	$(OBJ)/vidhrdw/psychic5.o $(OBJ)/drivers/psychic5.o \
	$(OBJ)/vidhrdw/ginganin.o $(OBJ)/drivers/ginganin.o \
	$(OBJ)/vidhrdw/megasys1.o $(OBJ)/drivers/megasys1.o \
	$(OBJ)/vidhrdw/cischeat.o $(OBJ)/drivers/cischeat.o \

$(OBJ)/vsystem.a: \
	$(OBJ)/vidhrdw/rpunch.o $(OBJ)/drivers/rpunch.o \
	$(OBJ)/vidhrdw/tail2nos.o $(OBJ)/drivers/tail2nos.o \
	$(OBJ)/vidhrdw/pipedrm.o $(OBJ)/drivers/pipedrm.o \
	$(OBJ)/vidhrdw/aerofgt.o $(OBJ)/drivers/aerofgt.o \

$(OBJ)/psikyo.a: \
	$(OBJ)/vidhrdw/psikyo.o $(OBJ)/drivers/psikyo.o \

$(OBJ)/leland.a: \
	$(OBJ)/machine/8254pit.o $(OBJ)/drivers/leland.o $(OBJ)/vidhrdw/leland.o $(OBJ)/sndhrdw/leland.o \
	$(OBJ)/drivers/ataxx.o \

$(OBJ)/orca.a: \
	$(OBJ)/vidhrdw/marineb.o $(OBJ)/drivers/marineb.o \
	$(OBJ)/vidhrdw/funkybee.o $(OBJ)/drivers/funkybee.o \
	$(OBJ)/vidhrdw/zodiack.o $(OBJ)/drivers/zodiack.o \
	$(OBJ)/vidhrdw/espial.o $(OBJ)/drivers/espial.o \
	$(OBJ)/vidhrdw/vastar.o $(OBJ)/drivers/vastar.o \

$(OBJ)/gaelco.a: \
	$(OBJ)/vidhrdw/gaelco.o $(OBJ)/drivers/gaelco.o \

$(OBJ)/kaneko.a: \
	$(OBJ)/vidhrdw/kaneko16.o $(OBJ)/drivers/kaneko16.o \
	$(OBJ)/vidhrdw/galpanic.o $(OBJ)/drivers/galpanic.o \
	$(OBJ)/vidhrdw/airbustr.o $(OBJ)/drivers/airbustr.o \

$(OBJ)/neogeo.a: \
	$(OBJ)/machine/neogeo.o $(OBJ)/machine/pd4990a.o $(OBJ)/vidhrdw/neogeo.o $(OBJ)/drivers/neogeo.o \

$(OBJ)/seta.a: \
	$(OBJ)/vidhrdw/hanaawas.o $(OBJ)/drivers/hanaawas.o \
	$(OBJ)/vidhrdw/seta.o $(OBJ)/sndhrdw/seta.o $(OBJ)/drivers/seta.o \

$(OBJ)/atlus.a: \
	$(OBJ)/vidhrdw/ohmygod.o $(OBJ)/drivers/ohmygod.o \
	$(OBJ)/vidhrdw/powerins.o $(OBJ)/drivers/powerins.o \

$(OBJ)/sun.a: \
	$(OBJ)/drivers/shanghai.o \
	$(OBJ)/vidhrdw/shangha3.o $(OBJ)/drivers/shangha3.o \

$(OBJ)/dooyong.a: \
	$(OBJ)/vidhrdw/gundealr.o $(OBJ)/drivers/gundealr.o \

$(OBJ)/nmk.a: \
	$(OBJ)/vidhrdw/macross.o $(OBJ)/drivers/macross.o \
	$(OBJ)/vidhrdw/bjtwin.o $(OBJ)/drivers/bjtwin.o \

$(OBJ)/other.a: \
	$(OBJ)/vidhrdw/spacefb.o $(OBJ)/drivers/spacefb.o \
	$(OBJ)/vidhrdw/blueprnt.o $(OBJ)/drivers/blueprnt.o \
	$(OBJ)/drivers/omegrace.o \
	$(OBJ)/vidhrdw/dday.o $(OBJ)/drivers/dday.o \
	$(OBJ)/vidhrdw/leprechn.o $(OBJ)/drivers/leprechn.o \
	$(OBJ)/vidhrdw/hexa.o $(OBJ)/drivers/hexa.o \
	$(OBJ)/vidhrdw/redalert.o $(OBJ)/sndhrdw/redalert.o $(OBJ)/drivers/redalert.o \
	$(OBJ)/machine/irobot.o $(OBJ)/vidhrdw/irobot.o $(OBJ)/drivers/irobot.o \
	$(OBJ)/machine/spiders.o $(OBJ)/vidhrdw/crtc6845.o $(OBJ)/vidhrdw/spiders.o $(OBJ)/drivers/spiders.o \
	$(OBJ)/machine/stactics.o $(OBJ)/vidhrdw/stactics.o $(OBJ)/drivers/stactics.o \
	$(OBJ)/vidhrdw/sharkatt.o $(OBJ)/drivers/sharkatt.o \
	$(OBJ)/vidhrdw/kingobox.o $(OBJ)/drivers/kingobox.o \
	$(OBJ)/vidhrdw/zerozone.o $(OBJ)/drivers/zerozone.o \
	$(OBJ)/vidhrdw/speedbal.o $(OBJ)/drivers/speedbal.o \
	$(OBJ)/vidhrdw/sauro.o $(OBJ)/drivers/sauro.o \
	$(OBJ)/vidhrdw/ambush.o $(OBJ)/drivers/ambush.o \
	$(OBJ)/vidhrdw/starcrus.o $(OBJ)/drivers/starcrus.o \
	$(OBJ)/vidhrdw/goindol.o $(OBJ)/drivers/goindol.o \
	$(OBJ)/drivers/dlair.o \
	$(OBJ)/vidhrdw/meteor.o $(OBJ)/drivers/meteor.o \
	$(OBJ)/vidhrdw/aztarac.o $(OBJ)/sndhrdw/aztarac.o $(OBJ)/drivers/aztarac.o \
	$(OBJ)/vidhrdw/mole.o $(OBJ)/drivers/mole.o \
	$(OBJ)/vidhrdw/gotya.o $(OBJ)/sndhrdw/gotya.o $(OBJ)/drivers/gotya.o \
	$(OBJ)/vidhrdw/mrjong.o $(OBJ)/drivers/mrjong.o \
	$(OBJ)/vidhrdw/polyplay.o $(OBJ)/sndhrdw/polyplay.o $(OBJ)/drivers/polyplay.o \
	$(OBJ)/vidhrdw/mermaid.o $(OBJ)/drivers/mermaid.o \
	$(OBJ)/vidhrdw/magix.o $(OBJ)/drivers/magix.o \
	$(OBJ)/drivers/royalmah.o \

COREOBJS += $(OBJ)/driver.o

# generated text files
TEXTS += gamelist.txt

gamelist.txt: $(EMULATOR)
	@echo Generating $@...
	@$(EMULATOR) -gamelistheader -noclones > gamelist.txt
	@$(EMULATOR) -gamelist -noclones | sort >> gamelist.txt
	@$(EMULATOR) -gamelistfooter >> gamelist.txt


