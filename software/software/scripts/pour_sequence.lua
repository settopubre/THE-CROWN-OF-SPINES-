-- 3DAI Engine - Concrete Pour Sequence Script
-- Demonstrates sequential construction phases via Lua bindings

engine_log("=== CONCRETE POUR SEQUENCE STARTED ===")
engine_log("Initializing site parameters...")

-- Step 1: Excavation
engine_log("Phase 1: Excavation starting...")
engine_log("  - Clearing topsoil to 12 inches depth")
engine_log("  - Grading subbase to specified slope")

-- Step 2: Gravel Base
engine_log("Phase 2: Placing gravel base...")
engine_log("  - Spreading 4-inch compacted gravel layer")
engine_log("  - Verifying compaction density > 95%")

-- Step 3: Formwork
engine_log("Phase 3: Setting formwork...")
engine_log("  - Installing 2x6 lumber perimeter forms")
engine_log("  - Staking every 24 inches on center")

-- Step 4: Rebar
engine_log("Phase 4: Tying rebar...")
engine_log("  - Laying #4 grid at 18-inch centers")
engine_log("  - Installing chairs at 4-foot spacing")

-- Step 5: Pour
engine_log("Phase 5: Pouring concrete...")
engine_log("  - Dispatching 4000 PSI mix trucks")
engine_log("  - Pouring from north to south in continuous operation")

-- Step 6: Finishing
engine_log("Phase 6: Finishing surface...")
engine_log("  - Screeding to form height")
engine_log("  - Bull floating and hand troweling")
engine_log("  - Cutting control joints at 10-foot intervals")

-- Step 7: Curing
engine_log("Phase 7: Curing...")
engine_log("  - Applying liquid membrane curing compound")
engine_log("  - Monitoring ambient temperature for frost protection")

engine_log("=== SEQUENCE COMPLETE ===")
engine_log("Total estimated time: 6.5 hours (weather permitting)")
