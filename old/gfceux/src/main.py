#!/usr/bin/python
# gfceux - Graphical launcher for fceux.
# Designed on Ubuntu, with platfrom independence in mind.
version = "2.2svn"
title = "gfceux"
# Copyright (C) 2008  Lukas Sabota <ltsmooth42 _at_ gmail.com>
##
"""
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

  # # # # # # # #
# Python imports

import sys
import os
import pickle
import shutil
#from optparse import OptionParser
#from config_parse import FceuxConfigParser
#import get_key
#from subprocess import Popen

try:
    import pygtk
    pygtk.require("2.0")
    import gtk
except ImportError:
    print "The PyGTK libraries cannot be found.\n\
    Ensure that PyGTK (>=2.12) is installed on this system.\n\
    On Debian based systems (like Ubuntu), try this command:\n\
    sudo apt-get install python-gtk2 libgtk2.0-0"


class GameOptions:
    # sound
    sound_check = True
    soundq_check = True
    soundrate_entry = "11000"
    soundbufsize_entry = "48"
  
    # video
    fullscreen_check = False
    xscale_spin = 2
    yscale_spin = 2
    bpp_combo = 32
  
    opengl_check = False
    autoscale_check = True
  
    # main
    extra_entry = ''
    romfile = ''
    moviefile = ''
    luafile = ''
  
    
    # network
    join_radio = False
    join_add = ''
    join_port = 4046
    join_pass = ''
    host_radio = False
    host_port = 4046
    host_pass = ''
    no_network_radio = True
    

def load_options():
    global options, optionsfile
    try:
        ifile = file(optionsfile, 'r')
        options = pickle.load(ifile)
        pickle.load(ifile)
    except:
        return
    ifile.close()

def save_options():
    global options, optionsfile
    if os.path.exists(os.path.dirname(optionsfile)) == 0:
        os.mkdir(os.path.dirname(optionsfile))
    ofile = open(optionsfile, 'w')
    pickle.dump(options, ofile)
    ofile.close()
    
def give_widgets():
    """
    give_widgets()
    
    This function takes data from the options struct and relays it to
    the GTK window
    """
    global options, widgets
    try:
        widgets.get_object("rom_entry").set_text(options.romfile)
        widgets.get_object("movie_entry").set_text(options.moviefile)
        widgets.get_object("lua_entry").set_text(options.luafile)
    
        # sound
        widgets.get_object("sound_check").set_active(options.sound_check)
        widgets.get_object("soundq_check").set_active(options.soundq_check)
        widgets.get_object("soundrate_entry").set_text(options.soundrate_entry)
        widgets.get_object("soundbufsize_entry").set_text(options.soundbufsize_entry)
    
        # video
        widgets.get_object("fullscreen_check").set_active(options.fullscreen_check)
        widgets.get_object("opengl_check").set_active(options.opengl_check)
        widgets.get_object("autoscale_check").set_active(options.autoscale_check)
        
        # set/unset sensitivity on manual scaling
        # TODO: idk if i like this really
        #if widgets.get_object("autoscale_check").get_active():
        #    widgets.get_object("scaling_frame").set_sensitive(False)
        #else:
        #    widgets.get_object("scaling_frame").set_sensitive(True)
        widgets.get_object("xscale_spin").set_value(options.xscale_spin)
        widgets.get_object("yscale_spin").set_value(options.yscale_spin)
    
        widgets.get_object("extra_entry").set_text(options.extra_entry)
        
        # Usability point:
        # Users will probably not want to remember their previous network setting.
        # Users may accidently be connecting to a remote server/hosting a game when
        # they were unaware.
        # No network is being set by default
        widgets.get_object("no_network_radio").set_active(True)
        widgets.get_object("join_add").set_text(options.join_add)
        widgets.get_object("join_port").set_value(float(options.join_port))
        widgets.get_object("join_pass").set_text(options.join_pass)
        widgets.get_object("host_port").set_value(float(options.host_port))
        widgets.get_object("host_pass").set_text(options.host_pass)

    except AttributeError:   
        # When new widgets are added, old pickle files might break.
        options = GameOptions()
        give_widgets()   

def set_options():
    """ 
    set_options()
  
    This function grabs all of the data from the GTK widgets
    and stores it in the options object.
    """
    options.romfile = widgets.get_object("rom_entry").get_text()
    options.moviefile = widgets.get_object("movie_entry").get_text()
    options.luafile = widgets.get_object("lua_entry").get_text()
  
    # sound
    options.sound_check = widgets.get_object("sound_check").get_active()
    options.soundq_check = widgets.get_object("soundq_check").get_active()
    options.soundrate_entry = widgets.get_object("soundrate_entry").get_text()
    options.soundbufsize_entry = widgets.get_object("soundbufsize_entry").get_text()
  
    # video
    options.fullscreen_check = widgets.get_object("fullscreen_check").get_active()
    options.opengl_check = widgets.get_object("opengl_check").get_active()
    options.autoscale_check = widgets.get_object("autoscale_check").get_active()
  
    options.xscale_spin = widgets.get_object("xscale_spin").get_value()
    options.yscale_spin = widgets.get_object("yscale_spin").get_value()
  
    options.extra_entry = widgets.get_object("extra_entry").get_text()
  
    options.join_radio = widgets.get_object("join_radio").get_active()
    options.host_radio = widgets.get_object("host_radio").get_active()
    options.no_network_radio = widgets.get_object("no_network_radio").get_active()
    options.join_add = widgets.get_object("join_add").get_text()
    options.join_port = int(widgets.get_object("join_port").get_value())
    options.join_pass = widgets.get_object("join_pass").get_text()
    options.host_port = widgets.get_object("host_port").get_value()
    options.host_pass = widgets.get_object("host_pass").get_text()
  

def find_binary(file):
    # first check the script directory
    if os.path.isfile(os.path.join(os.path.dirname(sys.argv[0]),file)):
        return os.path.join(os.path.dirname(sys.argv[0]), file)
  
    # if not in the script directory, check the $PATH 
    path = os.getenv('PATH')
    directories= []
    directory = ''
    # check for '$' so last entry is processed
    for x in path + '$':
        if x != ':' and x != '$':
            directory = directory + x
        else:
            directories.append(directory)
            directory = ''

    for x in directories:
        if os.path.isfile(os.path.join(x, file)):
            return os.path.join(x,file)

    return None
  
  
# # # # # # # # # # # # # # # # #
# Globals
options = None
configfile = os.getenv('HOME') + '/.fceux/fceux.cfg'
optionsfile = os.getenv('HOME') + '/.fceux/gfceux_options.dat'
widgets = None

class GfceuxApp:
    def __init__(self):
        self.fceux_binary = self.find_fceux()
        self.load_ui()
        self.create_config()

        options = GameOptions()
        load_options()
        give_widgets()
        try:
            gtk.main()
        except KeyboardInterrupt:
            sys.exit(0)
            
    def create_config(self):
        if os.path.exists(configfile) == False:
            # auto generate a default config by running fceux with no options
            os.system(find_binary("fceux"))

    def msg(self, text, use_gtk=False):
        """
        GfceuApp.msg()
    
        This function prints messages to the user.  This is generally used for status
        messages. If a GTK message_box is requried, the use_gtk flag can be enabled.
        """
        print text
        if use_gtk:
            msgbox = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_INFO,
                buttons=gtk.BUTTONS_CLOSE)
            msgbox.set_markup(text)
            msgbox.run()
            msgbox.destroy()

    def print_error(self, message, code, use_gtk=True, fatal=True):
        """
        GfceuApp.print_error()
    
        Presents the user with an error message and optionally quits the program.
        """
        print title + ' error code '+str(code)+': ' + message
        if use_gtk:
            msgbox = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_ERROR,
                buttons=gtk.BUTTONS_CLOSE)
            msgbox.set_markup(title + ' ERROR Code '+str(code)+':\n'+message)
            msgbox.run()
            msgbox.destroy()
        if fatal:
            sys.exit(code)
   
    def find_fceux(self):
        bin = find_binary('fceux')
        if bin == None:
            self.print_error('Could not find the fceux binary.\n\
                Ensure that fceux is installed and in the $PATH.\n', 4, True)
        else:
            self.msg('Using: ' + bin)
    
        return bin
  
    def load_ui(self):
        global widgets
        """ Search for the glade XML file and load it """
        # Check first in the directory of this script.
        if os.path.isfile('data/gfceux.glade'):
            glade_file = 'data/gfceux.glade'
        # Then check to see if its installed on a *nix system
        elif os.path.isfile(os.path.join(os.path.dirname(sys.argv[0]), '../share/gfceux/gfceux.glade')):
            glade_file = os.path.join(os.path.dirname(sys.argv[0]), '../share/gfceux/gfceux.glade')
        else:
            print 'ERROR.'
            print 'Could not find the glade UI file.'
            print 'Try reinstalling the application.'
            sys.exit(1)
    
        try:
            print "Using: " + glade_file
            widgets = gtk.Builder()
            widgets.add_from_file(glade_file)
            widgets.connect_signals(self)
        except:
            self.print_error("Couldn't load the UI data.", 24)
    
        widgets.get_object("main_window").show_all()
    
      
    def launch(self, rom_name, local=False):
        global options
        set_options()
    
        sound_options = ''
    
        if options.sound_check:
            sound_options += '--sound 1 '
        else:
            sound_options += '--sound 0 '

        if options.soundq_check:
            sound_options += '--soundq 1 '
        else:
            sound_options += '--soundq 0 '
      
        if options.soundrate_entry:
            sound_options += '--soundrate ' + options.soundrate_entry + ' '
    
        if options.soundbufsize_entry:
            sound_options += '--soundbufsize ' + options.soundbufsize_entry + ' '
    
        # video
        video_options = ''
        if options.fullscreen_check:
            video_options += '--fullscreen 1 '
        else:
            video_options += '--fullscreen 0 '
      
        if options.opengl_check:
            video_options += '--opengl 1 '
        else:
            video_options += '--opengl 0 '
    
        if options.autoscale_check:
            video_options += '--autoscale 1 '
        else:
            video_options += '--autoscale 0 '

        video_options += ' --xscale ' + str(options.xscale_spin)
        video_options += ' --yscale ' + str(options.yscale_spin)
        video_options += ' '
    
        # lua/movie
        other_options = ''
        if options.luafile:
            other_options += '--loadlua ' + options.luafile + ' '
        if options.moviefile:
            other_options += '--playmov ' + options.moviefile + ' '
          
    
        # Netplay is fucked right now
        if options.join_radio:
            if options.join_pass == '':
                netpass = ''
            else:
                netpass = '--pass ' + options.join_pass
            network = '--net ' + options.join_add +\
                ' --port '+ str(options.join_port) + ' ' + netpass
        else:
            network = ''
   
        if options.host_radio:
            if options.host_pass == '':
                netpass = ' '
            else:
                netpass = ' --pass ' + '"' + options.host_pass + '" '
            network = '--net localhost --port '+\
                str(options.host_port) + netpass + ' '
            network = ''
      
        if local:
            network = ''
   
    
        command =  self.fceux_binary + ' ' + sound_options + video_options +\
        network + other_options + options.extra_entry + ' '+ rom_name
        self.msg('Command: ' + command)

        
        if options.host_radio:
          xterm_binary = find_binary("xterm")
          if xterm_binary == None:
            gfceu_error("Cannot find xterm on this system.  You will not \n\
            be informed of server output.", 102, True, False)
            args = [self.server_binary]
          else:
            args = [xterm_binary, "-e", self.server_binary]
          args.append('--port')
          args.append(str(options.host_port))
          if options.host_pass:
            args.append("--password")
            args.append(options.host_pass)
          pid = Popen(args).pid
           
        widgets.get_object("main_window").hide()
    
        # os.system() is a blocker, so we must force
        # gtk to process our events.
        while gtk.events_pending():
          gtk.main_iteration_do()
    
        os.system(command)
        widgets.get_object("main_window").show()
    
        
        
        if options.host_radio:
          os.kill(pid, 9)
        
        
    ### Callbacks
    def launch_button_clicked(self, arg1):
        global options
        options.romfile = widgets.get_object("rom_entry").get_text()
        if widgets.get_object("rom_entry").get_text() == '':
            self.msg('Please specify a ROM to open in the main tab.', True)
            return
      
        self.launch('"'+ options.romfile +'"')
  
    def autoscale_check_toggled(self, menuitem, data=None):
        if widgets.get_object("autoscale_check").get_active():
            widgets.get_object("scaling_frame").set_sensitive(False)
        else:
            widgets.get_object("scaling_frame").set_sensitive(True)
    
    def about_button_clicked(self, menuitem, data=None):
        widgets.get_object("about_dialog").set_name('GFCE UltraX '+version)
        widgets.get_object("about_dialog").run()
        widgets.get_object("about_dialog").hide()
  
    def lua_browse_button_clicked(self, menuitem, data=None):
        global options
        set_options()
        chooser = gtk.FileChooserDialog("Open...",  None,
                gtk.FILE_CHOOSER_ACTION_OPEN,
  			    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
  			     gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        chooser.set_property("local-only", False)
        chooser.set_default_response(gtk.RESPONSE_OK)
    
        filter=gtk.FileFilter()
        filter.set_name("Lua scripts")
        filter.add_pattern("*.lua")
        chooser.add_filter(filter)
    
        filter = gtk.FileFilter()
        filter.set_name("All files")
        filter.add_pattern("*")
        chooser.add_filter(filter)
    
        if options.luafile == '':
            folder = os.getenv('HOME')
        else:
            folder = os.path.split(options.luafile)[0]
      
        chooser.set_current_folder (folder)
    
        response = chooser.run()
        chooser.hide()
    
        if response == gtk.RESPONSE_OK:
            if chooser.get_filename():
                x = chooser.get_filename()
                widgets.get_object("lua_entry").set_text(x)
                options.luafile = x
  
    def movie_browse_button_clicked(self, menuitem, data=None):
        global options
        set_options()
        chooser = gtk.FileChooserDialog("Open...",  None,
                gtk.FILE_CHOOSER_ACTION_OPEN,
  			    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
  			     gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        chooser.set_property("local-only", False)
        chooser.set_default_response(gtk.RESPONSE_OK)
    
        filter=gtk.FileFilter()
        filter.set_name("FM2 movies")
        filter.add_pattern("*.fm2")
        chooser.add_filter(filter)
    
        filter = gtk.FileFilter()
        filter.set_name("All files")
        filter.add_pattern("*")
        chooser.add_filter(filter)
    
        if options.moviefile == '':
            folder = os.getenv('HOME')
        else:
            folder = os.path.split(options.moviefile)[0]
      
        chooser.set_current_folder (folder)
    
        response = chooser.run()
        chooser.hide()
    
        if response == gtk.RESPONSE_OK:
            if chooser.get_filename():
                x = chooser.get_filename()
                widgets.get_object("movie_entry").set_text(x)
                options.moviefile = x
    
    def rom_browse_button_clicked(self, menuitem, data=None):
        global options
        set_options()
        chooser = gtk.FileChooserDialog("Open...",  None,
                gtk.FILE_CHOOSER_ACTION_OPEN,
  			    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
  			     gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        chooser.set_property("local-only", False)
        chooser.set_default_response(gtk.RESPONSE_OK)
    
        filter=gtk.FileFilter()
        filter.set_name("NES Roms")
        filter.add_mime_type("application/x-nes-rom")
        filter.add_mime_type("application/zip")
        filter.add_pattern("*.nes")
        filter.add_pattern("*.zip")
        chooser.add_filter(filter)
    
        filter = gtk.FileFilter()
        filter.set_name("All files")
        filter.add_pattern("*")
        chooser.add_filter(filter)
    
        if options.romfile == '':
            folder = os.getenv('HOME')
        else:
            folder = os.path.split(options.romfile)[0]
      
        chooser.set_current_folder (folder)
    
        response = chooser.run()
        chooser.hide()
    
        if response == gtk.RESPONSE_OK:
            if chooser.get_filename():
                x = chooser.get_filename()
                widgets.get_object("rom_entry").set_text(x)
                # reset lua and movie entries on rom change
                widgets.get_object("movie_entry").set_text("")
                widgets.get_object("lua_entry").set_text("")
                options.romfile = x
    
    # fix this global its ugly
    # specifies which NES gamepad we are configuring
    gamepad_config_no = "0"
    
    # This isn't used yet because it doesn't work with joysticks.
    def gamepad_clicked_new(self, widget, data=None):
        widgets.get_object("gamepad_config_window").show_all()

       

	"""
#Disabled for now
#TODO: Full joystick support
    def button_clicked(self, widget, data=None):
        prefix = "SDL.Input.GamePad." + self.gamepad_config_no
        d = {'right_button' : prefix + "Right",
         'left_button' : prefix + "Left",
         'up_button' : prefix + "Up",
         'down_button' : prefix + "Down",
         'select_button' : prefix + "Select",
         'start_button' : prefix + "Start",
         'a_button' : prefix + "A",
         'b_button' : prefix + "B",
         'turbo_a_button' : prefix + "TurboA",
         'turbo_b_button' : prefix + "TurboB"}
        
        if get_key.has_pygame == False:
            self.msg("Pygame could not be found on this system.  Gfceux will revert to the old configuration routine.", True)
            self.gamepad_clicked_old(widget)
        else:
            kg = get_key.KeyGrabber()
            key_tuple = kg.get_key()
            cp = FceuxConfigParser(configfile)
            cp.writeKey(prefix + "DeviceType", key_tuple[0])
            if key_tuple[0] == "Keyboard":
                print key_tuple
                cp.writeKey(d[widget.name], key_tuple[1])
            if key_tuple[0] == "Joystick":
                print key_tuple
                cp.writeKey(prefix + "DeviceNum", key_tuple[2])
                cp.writeKey(d[widget.name], key_tuple[1])	
	"""  
    def gamepad_clicked(self, widget, data=None):
        d = {'gp1_button' : "1",
            'gp2_button' : "2",
            'gp3_button' : "3",
            'gp4_button' : "4"}
        self.gamepad_config_no = d[widget.name]
        command = '--inputcfg gamepad' + self.gamepad_config_no
        self.launch(command, True)
    
    def gamepad_window_close(self, widget, data=None):
        widgets.get_object("gamepad_config_window").hide()
        return True

    def config_help_button_clicked(self, menuitem, data=None):
        msgbox = gtk.MessageDialog(parent=None, flags=0,
            type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_CLOSE)
        msgbox.set_markup("You should be able to figure it out from here.")
        msgbox.run()
        msgbox.hide()

    def join_radio_clicked(self, menuitem, data=None):
        global options
        widgets.get_object("join_frame").set_sensitive(True)
        widgets.get_object("host_frame").set_sensitive(False)
        options.join_radio = True
        options.host_radio = False
        options.no_network_radio = False
    
    def host_radio_clicked(self, menuitem, data=None):
        if widgets.get_object("host_radio").get_active():
            options.server_binary = find_binary('fceux-server')
      
        if options.server_binary == None:
            if os.name == 'nt':
                self.print_error("The fceux server software cannot be found. \n\
                    Ensure that it is installed in the same directory as \n\
                    GFCE Ultra.", 102, True, False)
        else:
            self.print_error("The fceux server software cannot be found on \n\
                this system.  Ensure that it is installed and in your path.",
                101, True, False)
            widgets.get_object("no_network_radio").set_active(True)
            options.no_network_radio = True
            return False

        widgets.get_object("join_frame").set_sensitive(False)
        widgets.get_object("host_frame").set_sensitive(True)
        options.join_radio = False
        options.host_radio = True
        options.no_network_radio = False

    def no_network_radio_clicked(self, menuitem, data=None):
        widgets.get_object("join_frame").set_sensitive(False)
        widgets.get_object("host_frame").set_sensitive(False)
        options.join_radio = False
        options.host_radio = False
        options.no_network_radio = True
  
    def end(self, menuitem, data=None):
        set_options()
        save_options()
        gtk.main_quit()

if __name__ == '__main__':
    app = GfceuxApp()
