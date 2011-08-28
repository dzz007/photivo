#!/usr/bin/env python

'''
mm extern photivo.py
Passes an image to Photivo

Author:
Michael Munzert (mike photivo org)
Bernd Schoeler (brother.john photivo org)

Version:
2011.01.29 Brother John: Ask user for photivo.exe and store in gimprc
2011.01.27 Brother John: Fixed failing execution of Photivo on Windows.
2011.01.02 mike: Initial version.

modelled after the trace plugin (lloyd konneker, lkk, bootch at nc.rr.com)

License:

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

The GNU Public License is available at
http://www.gnu.org/copyleft/gpl.html

'''

from gimpfu import *
from platform import system
import os
import subprocess
import Tkinter, tkFileDialog

def plugin_main(image, drawable, visible):
    # Copy so the save operations doesn't affect the original
    tempimage = pdb.gimp_image_duplicate(image)
    if not tempimage:
        raise RuntimeError

    # Use temp file names from gimp, it reflects the user's choices in gimp.rc
    tempfilename = pdb.gimp_temp_name("tif")
    if visible == 0:
        # Save in temporary.    Note: empty user entered file name
        tempdrawable = pdb.gimp_image_get_active_drawable(tempimage)
    else:
        # Get the current visible
        tempdrawable = pdb.gimp_layer_new_from_visible(image, tempimage, "visible")

    # !!! Note no run-mode first parameter, and user entered filename is empty string
    pdb.gimp_progress_set_text ("Saving a copy")
    pdb.gimp_file_save(tempimage, tempdrawable, tempfilename, "")

    # cleanup
    gimp.delete(tempimage)     # delete the temporary image


    # Platform dependent full command string for Photivo.
    if system() == "Linux":
        # We can assume Photivo can be called with a simple photivo.
        command = 'photivo -g "%s"' % (tempfilename)
    
    elif system() == "Windows":
        # There is no way to call Photivo without knowing exactly where it is installed.
        # So we ask the user for the path to photivo.exe and store it in the user's gimprc.
        cmdWindows = ""
        try:
            cmdWindows = pdb.gimp_gimprc_query("photivo-executable")
        except RuntimeError:        # Catch ExecutionError when the key is not found in gimprc
            pass

        if not os.path.exists(cmdWindows):
            root = Tkinter.Tk()
            root.withdraw()     # Hide the Tkinter main window so only the file dialog shows
            cmdWindows = tkFileDialog.askopenfilename(
                    parent = None, 
                    title = "Where is photivo.exe located?", 
                    filetypes = [('photivo.exe','photivo.exe')],
                    initialdir = "C:\\"
            )
            dummy = pdb.gimp_gimprc_set("photivo-executable", cmdWindows)
        
        command = '"%s" --load-and-delete "%s"' % (cmdWindows, tempfilename)    


    # Invoke Photivo.
    pdb.gimp_progress_set_text(command)
    pdb.gimp_progress_pulse()
    if system() == "Windows":
        child = subprocess.Popen(command)
    elif system() == "Linux":
        child = subprocess.Popen(command, shell = True)


register(
        "python_fu_mm_extern_photivo",
        "Pass the image to Photivo.",
        "Pass the image to Photivo.",
        "Michael Munzert (mike photivo org)",
        "Copyright 2011 Michael Munzert",
        "2011",
        "<Image>/Filters/MM-Filters/_Export to Photivo ...",
        "*",
        [ (PF_RADIO, "visible", "Layer:", 1, (("new from visible", 1),("current layer",0)))
        ],
        [],
        plugin_main,
)

main()
