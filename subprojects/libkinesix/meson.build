project (
    'libkinesix', 'c',
    version : '0.1.0',
    default_options: [
        'c_std=gnu11'
    ]
)

libkinesix_headers = [
    'include/kinesixd_daemon.h',
    'include/kinesixd_device.h',
    'include/kinesixd_device_p.h',
    'include/kinesixd_global.h'
]

libkinesix_sources = [
    'kinesixd_daemon.c',
    'kinesixd_device.c',
]

libkinesix_include_paths = include_directories(
    'include'
)

libkinesix = shared_library (
    meson.project_name (),
    sources: [
        libkinesix_headers,
        libkinesix_sources
    ],
    soversion : '0',
    include_directories : libkinesix_include_paths,
    dependencies : [
        dependency ('libinput'),
        dependency ('libudev'),
        dependency ('dbus-1'),
        dependency ('threads')
    ],
    install : true
)

libkinesix_dep = declare_dependency (
    include_directories : libkinesix_include_paths, 
    link_with : libkinesix
)

