rule("install-wdk", function()
    on_load(function()
        import("lib.detect.find_tool")

        local nuget = find_tool("nuget", { check = function(tool) os.run("%s help", tool) end })
        assert(nuget, "nuget not found!")

        if not os.isdir("packages") then
            print("installing wdk")
            os.execv(nuget.program, {
                "restore",
                path.join(os.projectdir(), "packages.config"),
                "-PackagesDirectory",
                path.join(os.projectdir(), "packages"),
            }, {})
        end
    end)
end)

add_rules("mode.debug", "mode.release")

add_rules("install-wdk")
includes("stormkit-driver/xmake.lua")
includes("driver/xmake.lua")
includes("qt-frontend/xmake.lua")
includes("cli-frontend/xmake.lua")
