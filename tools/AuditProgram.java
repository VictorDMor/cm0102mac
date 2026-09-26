// Export analysis metadata and entry-point pseudocode, not compilable source.
// @category CM0102
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.address.AddressIterator;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;

public class AuditProgram extends GhidraScript {
    @Override
    public void run() throws Exception {
        Path output = Path.of(getScriptArgs()[0]);
        Files.createDirectories(output);
        int functions = 0;
        try (PrintWriter writer = new PrintWriter(output.resolve("functions.tsv").toFile())) {
            writer.println("address\tname\tbody_bytes\texternal\tthunk");
            FunctionIterator iter = currentProgram.getFunctionManager().getFunctions(true);
            while (iter.hasNext() && !monitor.isCancelled()) {
                Function f = iter.next();
                writer.printf("%s\t%s\t%d\t%b\t%b%n", f.getEntryPoint(), f.getName(),
                    f.getBody().getNumAddresses(), f.isExternal(), f.isThunk());
                functions++;
            }
        }
        DecompInterface decompiler = new DecompInterface();
        try {
            decompiler.openProgram(currentProgram);
            AddressIterator entries = currentProgram.getSymbolTable().getExternalEntryPointIterator();
            try (PrintWriter writer = new PrintWriter(output.resolve("entry-pseudocode.c").toFile())) {
                writer.println("/* Ghidra analysis output. Not reconstructed or buildable source. */");
                while (entries.hasNext() && !monitor.isCancelled()) {
                    Function f = getFunctionAt(entries.next());
                    if (f == null) continue;
                    DecompileResults result = decompiler.decompileFunction(f, 30, monitor);
                    if (result.decompileCompleted()) writer.println(result.getDecompiledFunction().getC());
                    else writer.println("/* " + result.getErrorMessage() + " */");
                }
            }
        } finally { decompiler.dispose(); }
        try (PrintWriter writer = new PrintWriter(output.resolve("analysis.txt").toFile())) {
            writer.println("Program: " + currentProgram.getName());
            writer.println("Language: " + currentProgram.getLanguageID());
            writer.println("Functions detected: " + functions);
            writer.println("Static analysis only; game execution has not been validated.");
        }
        println("Exported " + functions + " detected functions.");
    }
}
