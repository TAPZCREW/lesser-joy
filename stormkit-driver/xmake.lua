target("stormkit-driver", function()
    set_languages("c++26")

    set_kind("moduleonly")

    add_files("modules/*.cppm", { public = true })
    add_files("src/*.cppm", "src/*.cpp")
    set_policy("build.c++.modules", true)
    set_policy("build.c++.modules.std", false)
    set_exceptions("no-cxx")

    add_headerfiles("includes/**.hpp", { public = true })
    add_includedirs("includes", { public = true })

    add_rules("wdk.env.kmdf")
end)
