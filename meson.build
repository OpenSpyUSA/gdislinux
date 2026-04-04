project(
  'gdis',
  'c',
  version: '1.0.0',
  meson_version: '>=0.61.0',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
  ],
)

gtk_target = get_option('gtk_target')
with_gui = get_option('gui')
with_grisu = get_option('grisu')

cc = meson.get_compiler('c')

if gtk_target == 'gtk2'
  base_dep_names = ['gtk+-2.0', 'gthread-2.0', 'gmodule-2.0']
  gui_dep_names = ['gtk+-2.0', 'gthread-2.0', 'gmodule-2.0', 'gtkglext-1.0']
  core_pkg_hint = 'sudo apt-get update && sudo apt-get install -y build-essential pkg-config meson ninja-build libgtk2.0-dev libgtkglext1-dev'
elif gtk_target == 'gtk3'
  base_dep_names = ['gtk+-3.0', 'gthread-2.0', 'gmodule-2.0']
  gui_dep_names = ['gtk+-3.0', 'gthread-2.0', 'gmodule-2.0', 'gl', 'glu', 'epoxy']
  core_pkg_hint = 'sudo apt-get update && sudo apt-get install -y build-essential pkg-config meson ninja-build libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev libepoxy-dev'
elif gtk_target == 'gtk4'
  base_dep_names = ['gtk4', 'gthread-2.0', 'gmodule-2.0']
  gui_dep_names = ['gtk4', 'gthread-2.0', 'gmodule-2.0', 'gl', 'glu', 'epoxy']
  core_pkg_hint = 'sudo apt-get update && sudo apt-get install -y build-essential pkg-config meson ninja-build libgtk-4-dev libgl1-mesa-dev libglu1-mesa-dev libepoxy-dev'
endif

dep_names = with_gui ? gui_dep_names : base_dep_names
gdis_deps = []
foreach dep_name : dep_names
  gdis_deps += dependency(dep_name)
endforeach

libm = cc.find_library('m', required: false)
if libm.found()
  gdis_deps += libm
endif

gdis_c_args = []
if with_gui
  gdis_c_args += ['-DWITH_GUI']
endif

if with_grisu
  gdis_deps += dependency('gsoapssl')
  gdis_deps += dependency('libssl')
  gdis_c_args += ['-DWITH_OPENSSL', '-DWITH_GRISU']
endif

if gtk_target == 'gtk4'
  warning('GTK4 support remains experimental. The Meson build mirrors the current GTK4 renewal branch, but some legacy interaction and overlay paths are still incomplete.')
endif

subdir('src')

install_data(
  'bin/gdis.elements',
  'bin/gdis.library',
  'bin/gdis.manual',
  install_dir: get_option('bindir'),
)

summary(
  {
    'GTK target': gtk_target,
    'GUI enabled': with_gui,
    'GRISU enabled': with_grisu,
    'Install helper': core_pkg_hint,
  },
  section: 'GDIS',
  bool_yn: true,
)
