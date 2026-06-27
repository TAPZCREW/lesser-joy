target("lesserjoy-cli", function()
    set_languages("c++26")
    add_files("src/*.cpp")
    set_policy("build.c++.modules", true)

    set_runtimes("c++_static")

    add_rules(stormkit_rule_prefix .. "stormkit::application")
    set_values("stormkit.components", { "stormkit", "log" })

    add_syslinks("OneCoreUAP")

    on_run(function(target)
        import("privilege.sudo")
        sudo.exec(target:targetfile())
    end)
end)
