project (
    'com.github.kicsyromy.kinesix', 'vala', 'c',
    version : '0.1.0',
    default_options : [
        'c_std=gnu11'
    ]
)

libkinesix_dep = dependency ('libkinesix', required : false)
if not libkinesix_dep.found ()
    libkinesix_proj = subproject ('libkinesix')
    libkinesix_dep = libkinesix_proj.get_variable ('libkinesix_dep')
endif

sources = [
    'src/Application.vala',
    'src/BackendDevice.vala',
    'src/BackendInterface.vala',
]

executable (
    meson.project_name (),
    sources,
    dependencies : [
        libkinesix_dep,
        dependency ('gtk+-3.0', method : 'pkg-config')
    ],
    install : true,
)

install_data(
    join_paths ('data', meson.project_name () + '.desktop'),
    install_dir : join_paths (get_option ('datadir'), 'applications')
)

install_data(
    join_paths ('data', meson.project_name () + '.appdata.xml'),
    install_dir : join_paths (get_option ('datadir'), 'metainfo')
)
