// Gets an i386 Objective-C 1 program ready for readable decompilation:
//  - applies the gcc PIC call fixups to ___i686.get_pc_thunk.* so the
//    decompiler knows the PIC register's value,
//  - labels selector, class and CFString references (sel_x, cls_X, cfstr_x),
//  - builds a structure per class from its ivars (from objc_dump.py JSON),
//  - gives every method the signature its type encoding describes, with self
//    typed as a pointer to its class structure.
// Script argument: path to objc-<binary>.json
// @category WeatherGlobe

import java.io.FileReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;

public class PrepareObjC extends GhidraScript {
	private DataTypeManager dtm;
	private final CategoryPath cat = new CategoryPath("/ObjC");
	private DataType id, sel, clsT, bool;
	private final Map<String, Structure> classStructs = new HashMap<>();
	private final Map<String, JsonObject> classes = new HashMap<>();

	@Override
	protected void run() throws Exception {
		dtm = currentProgram.getDataTypeManager();
		id = dtm.addDataType(new TypedefDataType(cat, "id", new PointerDataType(VoidDataType.dataType), dtm), null);
		sel = dtm.addDataType(new TypedefDataType(cat, "SEL", new PointerDataType(CharDataType.dataType), dtm), null);
		clsT = dtm.addDataType(new TypedefDataType(cat, "Class", new PointerDataType(VoidDataType.dataType), dtm), null);
		bool = dtm.addDataType(new TypedefDataType(cat, "BOOL", SignedCharDataType.dataType, dtm), null);

		fixupThunks();
		labelPointers("__message_refs", "sel_");
		labelPointers("__cls_refs", "cls_");
		labelCFStrings();
		setMsgSendSignatures();

		JsonArray modules = JsonParser.parseReader(new FileReader(getScriptArgs()[0])).getAsJsonArray();
		List<JsonObject> all = new ArrayList<>();
		for (JsonElement m : modules) {
			for (JsonElement c : m.getAsJsonObject().getAsJsonArray("classes")) {
				classes.put(c.getAsJsonObject().get("name").getAsString(), c.getAsJsonObject());
				all.add(c.getAsJsonObject());
			}
			for (JsonElement c : m.getAsJsonObject().getAsJsonArray("categories")) {
				all.add(c.getAsJsonObject());
			}
		}
		for (String name : classes.keySet()) {
			classStruct(name);
		}
		int typed = 0;
		for (JsonObject c : all) {
			String owner = c.has("class") ? c.get("class").getAsString() : c.get("name").getAsString();
			for (String key : new String[] { "imethods", "cmethods" }) {
				for (JsonElement me : c.getAsJsonArray(key)) {
					if (typeMethod(owner, key.equals("cmethods"), me.getAsJsonObject())) {
						typed++;
					}
				}
			}
		}
		println("typed " + typed + " methods, " + classStructs.size() + " class structures");
	}

	private void fixupThunks() throws Exception {
		for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
			String n = f.getName();
			int i = n.indexOf("get_pc_thunk");
			if (i < 0) {
				continue;
			}
			String reg = n.substring(n.length() - 2);
			f.setCallFixup("get_pc_thunk_" + reg);
			println("fixup " + n);
		}
	}

	private void labelPointers(String section, String prefix) throws Exception {
		for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
			if (!b.getName().equals(section)) {
				continue;
			}
			for (Address a = b.getStart(); a.compareTo(b.getEnd()) < 0; a = a.add(4)) {
				Address p = toAddr(getInt(a) & 0xffffffffL);
				String s = readCString(p);
				if (s != null) {
					createLabel(a, prefix + sanitize(s), true, SourceType.ANALYSIS);
				}
			}
		}
	}

	private void labelCFStrings() throws Exception {
		for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
			if (!b.getName().equals("__cfstring")) {
				continue;
			}
			for (Address a = b.getStart(); a.compareTo(b.getEnd()) < 0; a = a.add(16)) {
				String s = readCString(toAddr(getInt(a.add(8)) & 0xffffffffL));
				if (s != null) {
					String l = sanitize(s);
					createLabel(a, "cfstr_" + (l.length() > 48 ? l.substring(0, 48) : l), true,
						SourceType.ANALYSIS);
				}
			}
		}
	}

	private void setMsgSendSignatures() throws Exception {
		for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
			String n = f.getName().replaceFirst("^_+", "");
			if (!n.startsWith("objc_msgSend")) {
				continue;
			}
			List<ParameterImpl> ps = new ArrayList<>();
			if (n.endsWith("_stret")) {
				ps.add(new ParameterImpl("ret", new PointerDataType(VoidDataType.dataType), currentProgram));
			}
			ps.add(new ParameterImpl(n.startsWith("objc_msgSendSuper") ? "super" : "self",
				n.startsWith("objc_msgSendSuper") ? new PointerDataType(VoidDataType.dataType) : id,
				currentProgram));
			ps.add(new ParameterImpl("op", sel, currentProgram));
			DataType ret = n.endsWith("_fpret") ? DoubleDataType.dataType
					: n.endsWith("_stret") ? VoidDataType.dataType : id;
			f.updateFunction("__cdecl", new ReturnParameterImpl(ret, currentProgram), ps,
				FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
			f.setVarArgs(true);
		}
	}

	private String readCString(Address p) {
		try {
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < 400; i++) {
				byte c = getByte(p.add(i));
				if (c == 0) {
					return sb.toString();
				}
				sb.append((char) (c & 0xff));
			}
		}
		catch (Exception e) {
			// not mapped
		}
		return null;
	}

	private static String sanitize(String s) {
		String r = s.replaceAll("[^A-Za-z0-9_]", "_");
		return r.isEmpty() ? "empty" : r;
	}

	private Structure classStruct(String name) {
		Structure s = classStructs.get(name);
		if (s != null) {
			return s;
		}
		JsonObject c = classes.get(name);
		if (c == null) {
			return null;
		}
		StructureDataType st = new StructureDataType(cat, name, 0, dtm);
		classStructs.put(name, st);
		int size = c.get("instance_size").getAsInt();
		JsonArray ivars = c.getAsJsonArray("ivars");
		int base = ivars.size() > 0 ? ivars.get(0).getAsJsonObject().get("offset").getAsInt() : size;
		String sup = c.has("super") && !c.get("super").isJsonNull() ? c.get("super").getAsString() : null;
		Structure supS = sup != null ? classStruct(sup) : null;
		if (supS != null && supS.getLength() <= base) {
			st.add(supS, "super", null);
		}
		else if (base > 0) {
			st.add(new ArrayDataType(ByteDataType.dataType, base, 1), "super_" + sup, null);
		}
		for (int i = 0; i < ivars.size(); i++) {
			JsonObject iv = ivars.get(i).getAsJsonObject();
			int off = iv.get("offset").getAsInt();
			int end = i + 1 < ivars.size() ? ivars.get(i + 1).getAsJsonObject().get("offset").getAsInt() : size;
			DataType t;
			try {
				t = new Decoder(iv.get("type").getAsString()).next();
			}
			catch (Exception e) {
				t = null;
			}
			if (t == null || t.getLength() <= 0 || t.getLength() > end - off) {
				t = new ArrayDataType(ByteDataType.dataType, Math.max(1, end - off), 1);
			}
			while (st.getLength() < off) {
				st.add(ByteDataType.dataType, null, null);
			}
			st.add(t, iv.get("name").getAsString(), null);
		}
		while (st.getLength() < size) {
			st.add(ByteDataType.dataType, null, null);
		}
		Structure added = (Structure) dtm.addDataType(st, DataTypeConflictHandler.REPLACE_HANDLER);
		classStructs.put(name, added);
		return added;
	}

	private boolean typeMethod(String owner, boolean isClass, JsonObject m) throws Exception {
		Function f = getFunctionAt(toAddr(m.get("imp").getAsString()));
		if (f == null) {
			f = createFunction(toAddr(m.get("imp").getAsString()), null);
			if (f == null) {
				return false;
			}
		}
		Decoder d = new Decoder(m.get("types").getAsString());
		List<DataType> ts = new ArrayList<>();
		try {
			while (d.more()) {
				ts.add(d.next());
				d.skipDigits();
			}
		}
		catch (Exception e) {
			println("cannot decode " + m.get("types") + " for " + m.get("symbol"));
			return false;
		}
		String[] parts = m.get("sel").getAsString().split(":", -1);
		List<ParameterImpl> ps = new ArrayList<>();
		Structure cs = classStructs.get(owner);
		ps.add(new ParameterImpl("self", isClass || cs == null ? id : new PointerDataType(cs), currentProgram));
		ps.add(new ParameterImpl("_cmd", sel, currentProgram));
		for (int i = 3; i < ts.size(); i++) {
			String nm = i - 3 < parts.length && !parts[i - 3].isEmpty() ? parts[i - 3] : "arg" + (i - 2);
			ps.add(new ParameterImpl(sanitize(nm), ts.get(i), currentProgram));
		}
		DataType ret = ts.get(0);
		// i386 Darwin returns structs of up to 8 bytes in EAX:EDX, not
		// through a hidden pointer
		if (ret instanceof Composite && ret.getLength() <= 8) {
			ret = ret.getLength() <= 4 ? UnsignedIntegerDataType.dataType
					: UnsignedLongLongDataType.dataType;
		}
		f.updateFunction("__cdecl", new ReturnParameterImpl(ret, currentProgram), ps,
			FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
		return true;
	}

	/** Decodes Objective-C @encode strings into Ghidra data types. */
	private class Decoder {
		private final String s;
		private int p;

		Decoder(String s) {
			this.s = s;
		}

		boolean more() {
			return p < s.length();
		}

		void skipDigits() {
			while (p < s.length() && (Character.isDigit(s.charAt(p)) || s.charAt(p) == '-')) {
				p++;
			}
		}

		DataType next() throws Exception {
			// method type qualifiers
			while (p < s.length() && "rnNoORV".indexOf(s.charAt(p)) >= 0) {
				p++;
			}
			char c = s.charAt(p++);
			switch (c) {
				case 'c': return bool;
				case 'C': return UnsignedCharDataType.dataType;
				case 's': return ShortDataType.dataType;
				case 'S': return UnsignedShortDataType.dataType;
				case 'i': case 'l': return IntegerDataType.dataType;
				case 'I': case 'L': return UnsignedIntegerDataType.dataType;
				case 'q': return LongLongDataType.dataType;
				case 'Q': return UnsignedLongLongDataType.dataType;
				case 'f': return FloatDataType.dataType;
				case 'd': return DoubleDataType.dataType;
				case 'B': return BooleanDataType.dataType;
				case 'v': return VoidDataType.dataType;
				case '*': return new PointerDataType(CharDataType.dataType);
				case '#': return clsT;
				case ':': return sel;
				case '?': return new PointerDataType(VoidDataType.dataType);
				case '@': {
					if (p < s.length() && s.charAt(p) == '"') {
						int e = s.indexOf('"', p + 1);
						String cn = s.substring(p + 1, e);
						p = e + 1;
						Structure cs = classStructs.get(cn);
						return cs != null ? new PointerDataType(cs) : id;
					}
					if (p < s.length() && s.charAt(p) == '?') {
						p++;
					}
					return id;
				}
				case '^': {
					DataType t = next();
					return new PointerDataType(t == VoidDataType.dataType ? VoidDataType.dataType : t);
				}
				case 'b': {
					int st = p;
					skipDigits();
					return UnsignedIntegerDataType.dataType; // handled by caller for structs
				}
				case '[': {
					int st = p;
					skipDigits();
					int n = Integer.parseInt(s.substring(st, p));
					DataType t = next();
					p++; // ]
					return new ArrayDataType(t, Math.max(n, 1), t.getLength());
				}
				case '{':
				case '(': {
					char close = c == '{' ? '}' : ')';
					int eq = p;
					while (eq < s.length() && s.charAt(eq) != '=' && s.charAt(eq) != close) {
						eq++;
					}
					String name = s.substring(p, eq);
					p = eq;
					if (name.equals("?")) {
						name = "anon_" + Integer.toHexString(s.hashCode()) + "_" + p;
					}
					name = name.replaceFirst("^_+", "");
					DataType existing = dtm.getDataType(cat, name);
					if (s.charAt(p) == close) {
						p++;
						return existing != null ? existing : new PointerDataType(VoidDataType.dataType);
					}
					p++; // '='
					Composite comp = c == '{' ? new StructureDataType(cat, name, 0, dtm)
							: new UnionDataType(cat, name, dtm);
					comp.setPackingEnabled(true);
					int idx = 0;
					while (s.charAt(p) != close) {
						String fname = null;
						if (s.charAt(p) == '"') {
							int e = s.indexOf('"', p + 1);
							fname = s.substring(p + 1, e);
							p = e + 1;
						}
						if (s.charAt(p) == 'b') {
							p++;
							int st = p;
							skipDigits();
							int bits = Integer.parseInt(s.substring(st, p));
							((Structure) comp).addBitField(UnsignedIntegerDataType.dataType, bits,
								fname != null ? fname : "f" + idx, null);
						}
						else {
							DataType t = next();
							if (t.getLength() > 0) {
								comp.add(t, fname != null ? fname : "f" + idx, null);
							}
						}
						idx++;
					}
					p++;
					if (existing != null && existing.getLength() > 0) {
						return existing;
					}
					return dtm.addDataType(comp, DataTypeConflictHandler.KEEP_HANDLER);
				}
				default:
					throw new Exception("bad type char " + c + " in " + s);
			}
		}
	}
}
