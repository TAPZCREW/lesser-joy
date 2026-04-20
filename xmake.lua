target("nsw2ble")
  if is_arch("x64") then
    set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.x64.10.0.26100.4204/c")
  elseif is_arch("arm64") then
    set_values("wdk.sdkdir", "packages/Microsoft.Windows.WDK.arm64.10.0.26100.4204/c")
  end

  add_rules("wdk.driver", "wdk.env.kmdf")

  set_languages("c++26")
  set_exceptions("no-cxx")
  set_policy("build.c++.modules", true)
  set_policy("build.c++.modules.std", false)

  add_files("src/*.cppm", "src/*.cpp")
  add_files("src/*.rc")
  add_files("*.inf")
