#!/bin/sh
bannertool makebanner -i src/3ds/banner/atari800-banner.png \
	-a src/3ds/banner/silence.wav -o atari800-cia.bnr
bannertool makesmdh -s "atari800" -l "atari800 computer emulator (0.2.0)" -p "atari800 team" \
	-i src/3ds/banner/atari800-icon.png -o atari800-cia.smdh -r regionfree
makerom -f cia -elf atari800-3ds.elf -icon atari800-cia.smdh -banner atari800-cia.bnr \
	-desc app:4 -v -o atari800.cia -target t -exefslogo -rsf src/3ds/banner/atari800.rsf
