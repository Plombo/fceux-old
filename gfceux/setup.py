#!/usr/bin/env python
from distutils.core import setup
setup(name="gfceu",
		version="0.7svn",
		scripts = ['gfceu'],
		data_files=[('share/gfceu/',['gfceu.xml', 'gfceu_big.png', 'gfceu.png']),
                ('share/pixmaps/', ['gfceu.png']),
                ('share/man/man1/', ['gfceu.1']),
      			    ('share/applications/', ['gfceu.desktop'])],
		author = "Lukas Sabota",
		author_email = "punkrockguy318@comcast.net",
		url = "http://punkrockguy318.no-ip.org"

		)
