# -*- coding: utf-8 -*-
import sys, subprocess as proc
from tkinter import Tk, messagebox

SUCCESS = 0
ERROR   = 1

def main():
    if (sys.version_info.major < 3):
        error_box('Python too old. Precommit script needs at least Python v3.x.\n'
                  'You have v%s'%(sys.version))
    
    try:
        hgbranch = proc.check_output(['hg', 'branch'], universal_newlines=True)
        hgbranch = hgbranch.strip()
        
        proc.check_call(['hg', 'incoming', '-b', hgbranch])
    
    except proc.CalledProcessError as err:
        if err.returncode == 1:
            # no incoming changesets found, so this is in fact a success ;)
            return SUCCESS
        else:
            error_box('An error occurred while checking for incoming changesets. Commit aborted.\n'
                      'Details:\n%s'%(str(err)))
            return err.returncode
    
    if yesno_box('You did not pull before trying to commit but there are\n'
                 'new changesets waiting on the server.\n\n'
                 'Commit anyway?'):
        return SUCCESS
    else:
        return ERROR


def error_box(msg):
    Tk().withdraw()
    messagebox.showerror('Error', msg)

def yesno_box(msg):
    Tk().withdraw()
    return messagebox.askyesno('Warning', msg)


if __name__ == "__main__":
    sys.exit(main())
