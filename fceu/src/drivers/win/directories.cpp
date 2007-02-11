#include "common.h"
#include "main.h"
#include "window.h"
#include "gui.h"

/**
* Processes information from the Directories selection dialog after
* the dialog was closed.
*
* @param hwndDlg Handle of the dialog window.
**/
void CloseDirectoriesDialog(HWND hwndDlg)
{
	// Update the information from the screenshot naming checkbox
	if(IsDlgButtonChecked(hwndDlg, CHECK_SCREENSHOT_NAMES) == BST_CHECKED)
	{
		eoptions |= EO_SNAPNAME;
	}
	else
	{
		eoptions &= ~EO_SNAPNAME;
	}

	RemoveDirs();   // Remove empty directories.

	// Read the information from the edit fields and update the
	// necessary variables.
	for(unsigned int curr_dir = 0; curr_dir < NUMBER_OF_DIRECTORIES; curr_dir++)
	{
		LONG len;
		len = SendDlgItemMessage(hwndDlg, EDIT_CHEATS + curr_dir, WM_GETTEXTLENGTH, 0, 0);

		if(len <= 0)
		{
			if(directory_names[curr_dir])
			{
				free(directory_names[curr_dir]);
			}

			directory_names[curr_dir] = 0;
			continue;
		}

		len++; // Add 1 for null character.

		if( !(directory_names[curr_dir] = (char*)malloc(len))) //mbg merge 7/17/06 added cast
		{
			continue;
		}

		if(!GetDlgItemText(hwndDlg, EDIT_CHEATS + curr_dir, directory_names[curr_dir], len))
		{
			free(directory_names[curr_dir]);
			directory_names[curr_dir] = 0;
			continue;
		}

		if (!directoryExists(directory_names[curr_dir]))
		{
			const char* mask = "Error: Directory %s does not exist. Create this directory?";

			char* buffer = (char*)malloc(strlen(mask) + strlen(directory_names[curr_dir]) + 1);

			sprintf(buffer, mask, directory_names[curr_dir]);

			if ( MessageBox(hwndDlg, buffer, "FCE Ultra", MB_ICONERROR | MB_YESNO) == IDYES )
			{
				if (!CreateDirectory(directory_names[curr_dir], 0))
				{
					MessageBox(hwndDlg, "Error: Couldn't create directory. Please choose a different directory.", "FCE Ultra", MB_ICONERROR | MB_OK);
					free(buffer);
					return;
				}
			}
			else
			{
				free(buffer);
				return;
			}

			free(buffer);
		}

	}

	initDirectories();
	CreateDirs();   // Create needed directories.
	SetDirs();      // Set the directories in the core.

	EndDialog(hwndDlg, 0);
}

/**
* Callback function for the directories configuration dialog.
**/
static BOOL CALLBACK DirConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:

			// Initialize the directories textboxes
			for(unsigned int curr_dir = 0; curr_dir < NUMBER_OF_DIRECTORIES; curr_dir++)
			{
				SetDlgItemText(hwndDlg, EDIT_CHEATS + curr_dir, directory_names[curr_dir]);
			}

			// Check the screenshot naming checkbox if necessary
			if(eoptions & EO_SNAPNAME)
			{
				CheckDlgButton(hwndDlg, CHECK_SCREENSHOT_NAMES, BST_CHECKED);
			}

			CenterWindowOnScreen(hwndDlg);

			break;

		case WM_CLOSE:
		case WM_QUIT:
			CloseDirectoriesDialog(hwndDlg);
			break;

		case WM_COMMAND:
			if( !(wParam >> 16) )
			{
				if( (wParam & 0xFFFF) >= BUTTON_CHEATS && (wParam & 0xFFFF) <= BUTTON_CHEATS + NUMBER_OF_DIRECTORIES)
				{
					// If a directory selection button was pressed, ask the
					// user for a directory.

					static char *helpert[6] = {
						"Cheats",
						"Miscellaneous",
						"Nonvolatile Game Data",
						"Save States",
						"Screen Snapshots",
						"Base Directory"
					};

					char name[MAX_PATH];

					if(BrowseForFolder(hwndDlg, helpert[ ( (wParam & 0xFFFF) - BUTTON_CHEATS)], name))
					{
						SetDlgItemText(hwndDlg, EDIT_CHEATS + ((wParam & 0xFFFF) - BUTTON_CHEATS), name);
					}
				}
				else switch(wParam & 0xFFFF)
				{
					case CLOSE_BUTTON:
						CloseDirectoriesDialog(hwndDlg);
						break;
					case BTN_CANCEL:
						EndDialog(hwndDlg, 0);
						break;
				}
			}

	}

	return 0;
}

/**
* Shows the dialog for configuring the standard directories.
**/
void ConfigDirectories()
{
	DialogBox(fceu_hInstance, "DIRCONFIG", hAppWnd, DirConCallB);
}

