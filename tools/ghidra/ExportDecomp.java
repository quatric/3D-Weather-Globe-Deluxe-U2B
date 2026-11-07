// Writes Ghidra's decompilation of every function in the current program to
// <outdir>/<program>/<name>.c, plus an index.tsv (address, size, name, file).
// @category WeatherGlobe

import java.io.File;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Set;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class ExportDecomp extends GhidraScript {
	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		File out = new File(args[0], currentProgram.getName());
		out.mkdirs();

		DecompInterface ifc = new DecompInterface();
		DecompileOptions opts = new DecompileOptions();
		ifc.setOptions(opts);
		ifc.toggleCCode(true);
		ifc.toggleSyntaxTree(false);
		ifc.openProgram(currentProgram);

		Set<String> used = new HashSet<>();
		try (PrintWriter index = new PrintWriter(new File(out, "index.tsv"), "UTF-8")) {
			FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
			while (it.hasNext() && !monitor.isCancelled()) {
				Function f = it.next();
				if (f.isThunk() || f.isExternal()) {
					continue;
				}
				String name = f.getName(true);
				String file = name.replaceAll("[^A-Za-z0-9_+\\-\\[\\]]", "_");
				if (file.length() > 180) {
					file = file.substring(0, 180);
				}
				String base = file;
				for (int n = 2; !used.add(file.toLowerCase()); n++) {
					file = base + "~" + n;
				}
				file += ".c";

				DecompileResults r = ifc.decompileFunction(f, 120, monitor);
				String c = r.decompileCompleted() ? r.getDecompiledFunction().getC()
						: "/* decompile failed: " + r.getErrorMessage() + " */\n";
				try (PrintWriter w = new PrintWriter(new File(out, file), "UTF-8")) {
					w.printf("/* %s @ %s, %d bytes */%n", name, f.getEntryPoint(),
						f.getBody().getNumAddresses());
					w.print(c);
				}
				index.printf("%s\t%d\t%s\t%s%n", f.getEntryPoint(), f.getBody().getNumAddresses(),
					name, file);
			}
		}
		ifc.dispose();
	}
}
