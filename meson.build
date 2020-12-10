project('eagle', 'cpp',
    version : '1.0.0',
    license : 'MIT')

boost_dep = dependency('boost', modules : ['system', 'thread'])

glog_dep = dependency('glog')
gflags_dep = dependency('gflags')

include_dir = include_directories('include')

exe = executable('eagle',
                 ['src/main.cc'],
                 cpp_args : '-std=c++17',
                 include_directories : include_dir,
                 dependencies : [
                   boost_dep,
                   glog_dep,
                   gflags_dep
                 ])

gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')
gmock_dep = gtest_proj.get_variable('gmock_dep')

tests_src = [
  'tests/main_test.cc',
  'tests/dispatcher_test.cc',
  'tests/handler_registry_test.cc'
]

test_exec = executable('eagle_test', 
                       tests_src,
                       cpp_args : [
                         '-std=c++17',
                         '-fprofile-instr-generate',
                         '-fcoverage-mapping'
                       ],
                       link_args : ['-fprofile-instr-generate'],
                       include_directories : include_dir,
                       dependencies: [
                           boost_dep,
                           glog_dep,
                           gflags_dep,
                           gtest_dep,
                           gmock_dep
                       ])
