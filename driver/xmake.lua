if is_mode("debug") or is_mode("reldbg") then add_cxflags("clang::-ggdb3") end

option("wdk", { default = "C:/Program Files (x86)/Windows kits/10" })

rule("generate_cert", function()
    before_build(function(_)
        import("core.base.option")
        import("lib.detect.find_tool")
        import("privilege.sudo")
        import("core.cache.localcache")

        local cache = localcache.cache("wdk_cert")
        local have_cert = cache:get("have_cert")

        if not have_cert then
            local pwsh = find_tool("pwsh.exe")
            assert(pwsh, "pwsh not found!")

            local pwsh_args = {
                "-NoLogo",
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                path.translate("scripts/gen_certificate.ps1"),
            }

            if option.get("diagnosis") then
                print("running sudo " .. pwsh.program .. " " .. table.concat(pwsh_args, " "))
            end

            print("Generating certificate ---------")
            sudo.execv(pwsh.program, pwsh_args)

            cache:set("have_cert", true)
            cache:save()
        end
    end)

    on_clean(function(_)
        local cache = localcache.cache("wdk_cert")
        cache:clear()
        cache:save()
    end)
end)

target("lesserjoy-driver", function()
    set_languages("c++26")

    add_rules("generate_cert")
    add_rules("wdk.driver", "wdk.env.umdf", "wdk.sign")

    add_rules(stormkit_rule_prefix .. "stormkit::flags")
    set_values("stormkit.components", { "stormkit", "log" })

    add_files("src/*.rc")
    add_files("*.inx")

    add_files("src/*.cpp", "src/*.cppm")
    set_policy("build.c++.modules", true)

    set_runtimes("c++_static")

    add_packages("frozen", "unordered_dense", "nontype_functional")
    add_packages("stormkit", { components = { "core", "log" } })

    set_values("wdk.sdkdir", get_config("wdk"))

    set_values("wdk.sign.mode", "test")
    set_values("wdk.sign.store", "My")
    set_values("wdk.sign.digest_algorithm", "SHA256")
    set_values("wdk.sign.company", "lesserjoy")
    set_values("wdk.sign.machine_store", true)

    on_run(function(target)
        import("lib.detect.find_tool")
        import("core.base.option")
        import("core.project.config")
        import("core.project.project")
        import("core.base.task")
        import("privilege.sudo")

        local wdk = target:data("wdk")
        local inf_file = path.absolute(target:targetfile()):gsub("dll", "inf")

        local buildenvs = target:compiler("cxx"):runenvs()

        local pnputil = find_tool("pnputil", {
            check = function(tool) return os.isfile(tool) and os.isexec(tool) end,
        })
        assert(pnputil, "pnputil not found!")

        local arch = assert(config.arch(), "arch not found!")
        local devgen = path.join(wdk.sdkdir, "Tools", arch, "devgen.exe")
        if not os.isexec(devgen) then devgen = path.join(wdk.sdkdir, "Tools", wdk.sdkver, arch, "devgen.exe") end
        if not os.isexec(devgen) then devgen = path.join(wdk.sdkdir, "Tools", wdk.sdkver, "x86", "devgen.exe") end
        assert(os.isexec(devgen), "devgen not found!")

        -- local remove_devgen_args = {
        --     "/remove",
        --     "/instanceid",
        --     "Lesserjoy",
        -- }
        -- print("Removing old virtual hardware ---------")
        -- if option.get("verbose") then print("running", "sudo " .. devgen, table.concat(remove_devgen_args, " ")) end
        -- sudo.execv(devgen, remove_devgen_args)

        local pnputil_remove_args = {
            "/delete-driver",
            inf_file,
            "/uninstall",
        }
        print("Removing old driver ---------")
        if option.get("verbose") then
            print("running", "sudo " .. pnputil.program, table.concat(pnputil_remove_args, " "))
        end
        try({ function() sudo.execv(pnputil.program, pnputil_remove_args) end })
        catch({ function(...) end })

        -- local devgen_args = {
        --     "/add",
        --     "/hardwareid",
        --     "ROOT\\lesserjoy",
        --     "/instanceid",
        --     "Lesserjoy",
        -- }
        -- print("Creating virtual hardware ---------")
        -- if option.get("verbose") then print("running", "sudo " .. devgen, table.concat(devgen_args, " ")) end
        -- sudo.execv(devgen, devgen_args)

        local pnputil_args = {
            "/add-driver",
            inf_file,
            "/install",
        }
        print("Installing driver ---------")
        if option.get("verbose") then print("running", "sudo " .. pnputil.program, table.concat(pnputil_args, " ")) end
        sudo.execv(pnputil.program, pnputil_args)

        -- sudo.exec("windbgx -pn WUDFHost.exe")
    end)
end)
