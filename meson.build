project (
    'kinesix_daemon', 'c',
    version : '0.1.0'
)

kinesix_daemon_headers = [
    'src/include/kinesixd_daemon.h',
    'src/include/kinesixd_dbus_adaptor.h',
    'src/include/kinesixd_device.h',
    'src/include/kinesixd_device_marshaler.h',
    'src/include/kinesixd_device_p.h',
    'src/include/kinesixd_global.h'
]

kinesix_daemon_sources = [
    'src/kinesixd_daemon.c',
    'src/kinesixd_dbus_adaptor.c',
    'src/kinesixd_device.c',
    'src/kinesixd_device_marshaler.c',
    'src/main.c'
]

executable = executable (
    meson.project_name(),
    sources: [
        kinesix_daemon_sources,
        kinesix_daemon_headers
    ],
    include_directories : include_directories(
        'src/include'
    ),
    dependencies : [
        dependency('libinput'),
        dependency('libudev'),
        dependency('dbus-1'),
        dependency('threads')
    ]
)
