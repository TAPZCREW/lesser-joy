add_requires("hidapi")

target("lesserjoy-cli", function()
    set_languages("c++26")
    add_files("src/*.cpp", "src/*.cppm")
    set_policy("build.c++.modules", true)

    add_packages("frozen", "unordered_dense", "nontype_functional", "status-code", "mdspan")
    add_rules(stormkit_rule_prefix .. "stormkit::application")
    set_values("stormkit.components", { "stormkit", "log" })
    add_packages("hidapi")

    add_syslinks("OneCoreUAP")
end)
