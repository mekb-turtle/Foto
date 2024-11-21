project('Foto', 'c',
  version: '3.0.1',
  license: 'MPL-2.0',
  default_options: ['warning_level=3'])

# define source files
src = files('src/main.c', 'src/arg.c', 'src/arg.h', 'src/image.c', 'src/image.h', 'src/util.c', 'src/util.h')

# define project metadata
url = 'https://github.com/mekb-turtle/Foto'
name = meson.project_name()
version = meson.project_version()

add_project_arguments(
  f'-DPROJECT_NAME="@name@"',
  f'-DPROJECT_VERSION="@version@"',
  f'-DPROJECT_URL="@url@"',
  language : 'c')

exe = executable('foto', sources: src, install: true, dependencies: [
  dependency('SDL2'),
  dependency('SDL2_image'),
])
