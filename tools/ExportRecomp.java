// Export the analyzed retail program for ahead-of-time compilation experiments.
// No emulation or game execution occurs in this exporter.
// @category CM0102
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import com.google.gson.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.zip.GZIPOutputStream;

public class ExportRecomp extends GhidraScript {
    private JsonArray node(Varnode value) {
        if (value == null) return null;
        JsonArray n = new JsonArray();
        n.add(value.getAddress().getAddressSpace().getName());
        n.add(Long.toUnsignedString(value.getOffset()));
        n.add(value.getSize());
        return n;
    }

    @Override public void run() throws Exception {
        if (!currentProgram.getLanguageID().toString().equals("PowerPC:BE:32:default")) {
            throw new IllegalStateException("Expected analyzed 32-bit big-endian PowerPC program");
        }
        Path output = Path.of(getScriptArgs()[0]);
        // Explicit indirect-branch destinations observed in the native probe
        // can fill gaps left by automatic analysis. Use a fresh export directory
        // and analyzeHeadless -readOnly; original program bytes are never changed.
        JsonArray discoveryRoots = new JsonArray();
        for (int i = 1; i < getScriptArgs().length; i++) {
            long offset = Long.decode(getScriptArgs()[i]);
            Address address = toAddr(offset);
            MemoryBlock block = currentProgram.getMemory().getBlock(address);
            if ((offset & 3) != 0 || block == null || !block.isExecute())
                throw new IllegalArgumentException("Discovery root is not aligned executable code: " + address);
            if (getInstructionAt(address) == null && !disassemble(address))
                throw new IllegalStateException("Cannot disassemble original branch destination: " + address);
            if (getInstructionAt(address) == null)
                throw new IllegalStateException("Discovery root has no instruction: " + address);
            discoveryRoots.add(offset);
        }
        Files.createDirectories(output);
        Files.deleteIfExists(output.resolve("program.json"));
        Gson gson = new Gson();
        JsonObject metadata = new JsonObject();
        metadata.addProperty("formatVersion", 1);
        metadata.addProperty("language", currentProgram.getLanguageID().toString());
        metadata.addProperty("executableSHA256", currentProgram.getExecutableSHA256());
        metadata.addProperty("ghidraVersion", ghidra.framework.Application.getApplicationVersion());
        metadata.add("explicitDiscoveryRoots", discoveryRoots);
        JsonArray segments = new JsonArray();
        int segmentIndex = 0;
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            JsonObject segment = new JsonObject();
            segment.addProperty("name", block.getName());
            segment.addProperty("address", block.getStart().getOffset());
            segment.addProperty("size", block.getSize());
            segment.addProperty("initialized", block.isInitialized());
            segment.addProperty("execute", block.isExecute());
            if (block.isInitialized()) {
                String filename = "segment-" + segmentIndex + ".bin";
                segment.addProperty("file", filename);
                try (OutputStream stream = Files.newOutputStream(output.resolve(filename))) {
                    byte[] buffer = new byte[65536];
                    long done = 0;
                    while (done < block.getSize()) {
                        int count = (int)Math.min(buffer.length, block.getSize() - done);
                        block.getBytes(block.getStart().add(done), buffer, 0, count);
                        stream.write(buffer, 0, count);
                        done += count;
                    }
                }
            }
            segments.add(segment);
            segmentIndex++;
        }
        metadata.add("segments", segments);
        JsonObject registers = new JsonObject();
        for (Register register : currentProgram.getLanguage().getRegisters()) {
            JsonArray description = new JsonArray();
            description.add(register.getOffset());
            description.add(register.getMinimumByteSize());
            registers.add(register.getName(), description);
        }
        metadata.add("registers", registers);
        JsonArray symbols = new JsonArray();
        SymbolIterator symbolIterator = currentProgram.getSymbolTable().getAllSymbols(true);
        while (symbolIterator.hasNext()) {
            Symbol symbol = symbolIterator.next();
            if (symbol.getName().startsWith("FUN_") || symbol.getName().startsWith("LAB_") ||
                symbol.getName().startsWith("DAT_")) continue;
            JsonObject item = new JsonObject();
            item.addProperty("name", symbol.getName(true));
            item.addProperty("space", symbol.getAddress().getAddressSpace().getName());
            item.addProperty("address", symbol.getAddress().getOffset());
            symbols.add(item);
        }
        metadata.add("symbols", symbols);
        JsonObject userops = new JsonObject();
        for (int i = 0; i < currentProgram.getLanguage().getNumberOfUserDefinedOpNames(); i++) {
            userops.addProperty(Integer.toString(i), currentProgram.getLanguage().getUserDefinedOpName(i));
        }
        metadata.add("userops", userops);
        Map<String, Integer> counts = new TreeMap<>();
        int instructionCount = 0;
        try (Writer writer = new BufferedWriter(new OutputStreamWriter(new GZIPOutputStream(
                Files.newOutputStream(output.resolve("instructions.jsonl.gz"))), StandardCharsets.UTF_8))) {
            InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
            while (instructions.hasNext() && !monitor.isCancelled()) {
                Instruction instruction = instructions.next();
                JsonObject entry = new JsonObject();
                entry.addProperty("address", instruction.getAddress().getOffset());
                entry.addProperty("size", instruction.getLength());
                entry.addProperty("assembly", instruction.toString());
                entry.addProperty("flow", instruction.getFlowType().toString());
                Address fallthrough = instruction.getFallThrough();
                if (fallthrough != null) entry.addProperty("fallthrough", fallthrough.getOffset());
                JsonArray flows = new JsonArray();
                for (Address flow : instruction.getFlows()) flows.add(flow.getOffset());
                entry.add("flows", flows);
                JsonArray operations = new JsonArray();
                for (PcodeOp op : instruction.getPcode()) {
                    JsonArray operation = new JsonArray();
                    operation.add(op.getMnemonic());
                    operation.add(node(op.getOutput()));
                    for (Varnode input : op.getInputs()) operation.add(node(input));
                    operations.add(operation);
                    counts.merge(op.getMnemonic(), 1, Integer::sum);
                }
                entry.add("ops", operations);
                writer.write(gson.toJson(entry));
                writer.write('\n');
                instructionCount++;
            }
        }
        if (monitor.isCancelled()) throw new IOException("Export interrupted; do not compile partial output");
        metadata.addProperty("instructionCount", instructionCount);
        metadata.add("operationCounts", gson.toJsonTree(counts));
        Files.writeString(output.resolve("program.json"), new GsonBuilder().setPrettyPrinting().create().toJson(metadata));
        println("Exported " + instructionCount + " analyzed instructions and " + segments.size() + " memory blocks.");
    }
}
