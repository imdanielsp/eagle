project('eagle', 'cpp',
    version : '1.0.0',
    license : 'MIT')

boost_dep = dependency('boost', modules : ['system', 'thread'])

include_dir = include_directories('include')

src = [
  'src/app.cc',
  'src/common.cc',
  'src/connection.cc',
  'src/dispatcher.cc',
  'src/handler_registry.cc',
  'src/handler.cc',
  'src/request.cc',
  'src/resource_matcher.cc',
  'src/request_arguments.cc'
]

lib = library('eagle',
              [src],
              version: '1.0.0',
              soversion : '0',
              cpp_args : [
                '-std=c++17',
              ],
              include_directories : include_dir,
              dependencies : [
                   boost_dep
              ])

exe = executable('eagle_example',
                 'examples/main.cc',
                 cpp_args : [
                   '-std=c++17',
                 ],
                 include_directories : include_dir,
                 link_with : lib)

benchmark = executable('eagle_benchmark',
                       'examples/benchmark.cc',
                       cpp_args : [
                         '-std=c++17'
                       ],
                      include_directories : include_dir,
                      link_with : lib)

gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')
gmock_dep = gtest_proj.get_variable('gmock_dep')

tests_src = [
  'tests/main_test.cc',
  'tests/dispatcher_test.cc',
  'tests/handler_test.cc',
  'tests/handler_registry_test.cc',
  'tests/resource_matcher_test.cc',
  'tests/request_arguments_test.cc',
  'tests/response_test.cc'
]

test_exec = executable('eagle_test', 
                       [tests_src],
                       cpp_args : [
                         '-std=c++17',
                         '-fprofile-instr-generate',
                         '-fcoverage-mapping'
                       ],
                       link_with : lib,
                       link_args : ['-fprofile-instr-generate'],
                       include_directories : include_dir,
                       dependencies: [
                           gtest_dep,
                           gmock_dep
                       ])
