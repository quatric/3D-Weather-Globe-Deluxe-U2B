// Runs before analysis. gcc's i386 PIC code loads its base address with
// "call next; next: pop %reg", which Ghidra takes for a call to a new
// function, splitting every function that touches a global in two. In
// Ghidra's copy of memory (the file isn't touched) each such call becomes the
// equivalent "push <next>", so the base is a constant and globals resolve.
// @category WeatherGlobe

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;

public class FixPicCalls extends GhidraScript {
	@Override
	protected void run() throws Exception {
		int patched = 0;
		for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
			if (!b.isExecute() || !b.isInitialized()) {
				continue;
			}
			long size = b.getSize();
			byte[] bytes = new byte[(int) size];
			b.getBytes(b.getStart(), bytes);
			for (int i = 0; i + 6 <= bytes.length; i++) {
				if ((bytes[i] & 0xff) == 0xe8 && bytes[i + 1] == 0 && bytes[i + 2] == 0
						&& bytes[i + 3] == 0 && bytes[i + 4] == 0
						&& (bytes[i + 5] & 0xf8) == 0x58) {
					Address at = b.getStart().add(i);
					long next = at.getOffset() + 5;
					byte[] push = { 0x68, (byte) next, (byte) (next >> 8), (byte) (next >> 16),
						(byte) (next >> 24) };
					currentProgram.getMemory().setBytes(at, push);
					patched++;
					i += 5;
				}
			}
		}
		println("patched " + patched + " PIC base calls");
	}
}
