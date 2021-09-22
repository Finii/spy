# Build specification

project('spy', [ 'cpp' ],
    default_options : [ 'cpp_std=c++14' ],
    meson_version : '>=0.50.0',
)

deps = [
    dependency('threads'),
    dependency('boost', modules : [ 'system', 'regex' ]),
]

sources = files(
    'spy.cc',
)

executable('spy', sources,
    dependencies : deps,
    install : true,
)

# vi:ts=4:sw=4:sts=4:et:ft=conf
