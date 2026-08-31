' AWG Function Generator — hidden launcher (no console window).
' Double-click this file to start the UI. It runs the windowless Python
' interpreter (pythonw) so no Command Prompt window appears.
Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

scriptDir = fso.GetParentFolderName(WScript.ScriptFullName)
q = Chr(34)

shell.Run "pyw -3.13 " & q & scriptDir & "\awg_ui.py" & q, 0, False
