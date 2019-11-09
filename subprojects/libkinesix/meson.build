project (
    'kinesix_daemon', 'c',
    version : '0.1.0',
    default_options: [
        'c_std=gnu11'
    ],
)

kinesix_daemon_headers = [
    'include/kinesixd_daemon.h',
    'include/kinesixd_dbus_adaptor.h',
    'include/kinesixd_device.h',
    'include/kinesixd_device_marshaler.h',
    'include/kinesixd_device_p.h',
    'include/kinesixd_global.h'
]

kinesix_daemon_sources = [
    'kinesixd_daemon.c',
    'kinesixd_dbus_adaptor.c',
    'kinesixd_device.c',
    'kinesixd_device_marshaler.c',
]

kinesix_daemon_lib = shared_library (
    meson.project_name(),
    sources: [
        kinesix_daemon_headers,
        kinesix_daemon_sources
    ],
    soversion : '0',
    include_directories : include_directories(
        'include'
    ),
    dependencies : [
        dependency('libinput'),
        dependency('libudev'),
        dependency('dbus-1'),
        dependency('threads')
    ]
)
