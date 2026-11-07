// Runs after analysis. The demangler types imported C++ methods from their
// mangled names, which don't include the implicit "this", so in every call to
// one the object is taken for the first argument and the last argument is
// dropped. This adds "this" in front of the parameters of each imported
// function that is a non-static member, as listed (Class::method, one per
// line) in the file given as the script argument.
// @category WeatherGlobe

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.Variable;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;

public class FixThisParams extends GhidraScript {
	@Override
	protected void run() throws Exception {
		Set<String> members = new HashSet<>(Files.readAllLines(Paths.get(getScriptArgs()[0])));
		int fixed = 0;
		FunctionIterator it = currentProgram.getFunctionManager().getExternalFunctions();
		while (it.hasNext()) {
			Function f = it.next();
			Namespace ns = f.getParentNamespace();
			if (ns == null || ns.isGlobal()) {
				continue;
			}
			String key = ns.getName() + "::" + f.getName();
			if (!members.contains(key)) {
				continue;
			}
			Parameter[] old = f.getParameters();
			if (old.length > 0 && "this".equals(old[0].getName())) {
				continue;
			}
			DataType cls = currentProgram.getDataTypeManager().getDataType("/Demangler/"
				+ ns.getName(true).replace("::", "/"));
			DataType ptr = new PointerDataType(cls != null ? cls : VoidDataType.dataType);
			List<Variable> params = new ArrayList<>();
			params.add(new ParameterImpl("this", ptr, currentProgram));
			for (Parameter p : old) {
				params.add(new ParameterImpl(p.getName(), p.getDataType(), currentProgram));
			}
			try {
				f.replaceParameters(params, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
					true, SourceType.USER_DEFINED);
				fixed++;
			}
			catch (Exception e) {
				println("could not fix " + key + ": " + e.getMessage());
			}
		}
		println("added this to " + fixed + " imported methods");
	}
}
