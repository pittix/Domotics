#
#Creates the gui on the raspberry so that the user can control it via the raspberry
#
#
import tkinter as tk
from tkinter import ttk as ttk
from tkinter import N,E,S,W

class MainInterface(tk.Frame):
    def __init__(self,parent):
        tk.Frame.__init__(parent) #
        self.parent=parent;
        self.initialize()

    def initialize(self):
        """creates the main interface, with the status and some buttons to access the other menu, like the config one or the programming or the status"""
        # self.parent.grid(column=3, row=3,sticky=(N,E,S,W))
        ttk.Button(self, text="Settings", command=self.quit).grid(column=3, row=3, sticky=E)
#example
class Application(tk.Frame):
    def __init__(self, master=None):
        tk.Frame.__init__(self, master)
        self.grid(sticky=tk.N+tk.S+tk.E+tk.W) # this is the maximum size something can have
        self.createWidgets()

    def createWidgets(self):
        top=self.winfo_toplevel()
        top.rowconfigure(0, weight=1)
        top.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.quitButton = tk.Button(self, text='Quit',
            command=self.quit)

        self.quitButton.grid(row=0, column=0,sticky=tk.N+tk.E)
        self.quitButton.grid()
        self.checkBstate= tk.IntVar();
        self.checkB = tk.Checkbutton(self,text="press me",anchor=tk.N+tk.W,variable=self.checkBstate,onvalue=1,offvalue=0)
        self.control = tk.Button(self, text='Checkbutton status',            command=self.check,height=2,width=2)
        self.checkB.grid(row=1,column=1,sticky=tk.W+tk.S)
        self.control.grid(row=1,column=0,sticky=tk.E+tk.S)

        optionList = ('train', 'plane', 'boat')
        self.v = tk.StringVar()
        self.v.set(optionList[0])
        self.v = tk.StringVar()
        self.om = tk.OptionMenu(self, self.v,label='Select one option', *optionList)
        self.om.grid(row=0,column=0,sticky=tk.N+tk.W)


    def check(self):
        print(self.checkBstate.get())


app = Application()
app.master.title('Sample application')
app.mainloop()

#
#
# if(__name__ == "__main__" ):
#     app=MainInterface(None)
#     app.title("home automation")
#
#     app.mainloop()
