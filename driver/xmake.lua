target("lesser-joy-driver", function()
    set_languages("c++26")

    add_deps("stormkit-driver")

    add_files("src/*.rc")
    add_files("*.inf")
    add_rules("wdk.driver", "wdk.env.kmdf")

    add_files("src/*.cpp", "src/*.cppm")
    set_policy("build.c++.modules", true)
    set_policy("build.c++.modules.std", false)
    set_exceptions("no-cxx")

    if is_arch("x64") then
        set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.x64.10.0.26100.4204/c")
    elseif is_arch("arm64") then
        set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.arm64.10.0.26100.4204/c")
    end
end)
