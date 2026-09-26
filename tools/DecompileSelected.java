// Read-only, targeted inspection while implementing native API boundaries.
// @category CM0102
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.util.*;

public class DecompileSelected extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        Path output = Path.of(args[0]);
        Files.createDirectories(output);
        DecompInterface decompiler = new DecompInterface();
        Set<String> done = new HashSet<>();
        try {
            decompiler.openProgram(currentProgram);
            for (int i = 1; i < args.length; i++) {
                if (args[i].equals("--temporary-functions")) continue;
                Function function = getFunctionContaining(toAddr(Long.parseUnsignedLong(args[i].replace("0x", ""), 16)));
                if (function == null && Arrays.asList(args).contains("--temporary-functions")) {
                    // Invoke only with analyzeHeadless -readOnly: this function
                    // discovery is discarded with the temporary program view.
                    function = createFunction(toAddr(Long.parseUnsignedLong(args[i].replace("0x", ""), 16)), null);
                }
                if (function == null) throw new IllegalStateException("No analyzed function at " + args[i]);
                String address = function.getEntryPoint().toString();
                if (!done.add(address)) continue;
                var result = decompiler.decompileFunction(function, 30, monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                Files.writeString(output.resolve(address + ".c"), result.getDecompiledFunction().getC());
                println("Exported " + address);
            }
        } finally { decompiler.dispose(); }
    }
}
