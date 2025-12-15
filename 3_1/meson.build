project('cp', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=gnu23']
)

src_files = [
  'src/main.c',
  'src/threadpool.c',
  'src/log.c',
  'src/copy.c'
]

threads = dependency('threads')

executable('cp',
  src_files,
  # cpp_args: build_flags,
  dependencies: [ threads ] 
)
