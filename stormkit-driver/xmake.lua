target("stormkit-driver", function()
    set_languages("c++26")

    set_kind("moduleonly")

    add_files("modules/*.cppm", { public = true })
    add_files("src/*.cppm", "src/*.cpp")
    set_policy("build.c++.modules", true)
    set_policy("build.c++.modules.std", false)
    set_exceptions("no-cxx")

    add_headerfiles("stormkit/(includes/*.hpp)")
    add_includedirs("stormkit")

    add_rules("wdk.env.kmdf")

    -- if is_arch("x64") then
    --     set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.x64.10.0.26100.4204/c")
    -- elseif is_arch("arm64") then
    --     set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.arm64.10.0.26100.4204/c")
    -- end
end)
