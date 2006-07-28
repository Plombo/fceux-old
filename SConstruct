CCFLAGS= ''
LINKFLAGS = ''

env = Environment(CCFLAGS=CCFLAGS, LINKFLAGS=LINKFLAGS)
conf = Configure(env)
if not conf.CheckLib('SDL'):
  print 'Did not find libSDL.a or SDL.lib, exiting!'
  Exit(1)
env = conf.Finish()
SConscript(['src/SConscript'])
