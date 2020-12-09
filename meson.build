project('mmDNS', 'cpp',
    version : '1.0.0',
    license : 'MIT')

boost_dep = dependency('boost', modules : ['system', 'thread'])

glog_dep = dependency('glog')
gflags = dependency('gflags')

src = [

]

exe = executable('eagle',
                 ['src/main.cc', src],
                 cpp_args : '-std=c++17',
                 dependencies : [boost_dep, glog_dep, gflags])

# gtest_dep = dependency('gtest', main : true, required : true)
# gmock_dep = dependency('gmock', main : true, required : true)

# tests_src = [
#   'tests/test_main.cc'
# ]

# test_exec = executable('eagle_test', 
#                        [src, dns_decoder_src, tests_src],
#                        cpp_args : '-std=c++17',
#                        dependencies: [
#                            gtest_dep,
#                            gmock_dep
#                        ])
