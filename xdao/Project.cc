/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000  Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "xcdrdao.h"
#include "gcdmaster.h"
#include "DeviceConfDialog.h"
#include "RecordGenericDialog.h"
#include "Project.h"

#include <gnome.h>

Project::Project(int number) : Gnome::App("gcdmaster", "Gnome CD Master")
{
  vector<Gnome::UI::Info> menus, newMenuTree, fileMenuTree, actionsMenuTree;
  vector<Gnome::UI::Info> settingsMenuTree, helpMenuTree, windowsMenuTree;

  {
    using namespace Gnome::UI;
    // File->New menu
    newMenuTree.push_back(Item(Icon(GNOME_STOCK_MENU_NEW),
								N_("Audio CD"),
								slot(this, &Project::newAudioCDProject),
								N_("New Audio CD")));

    // File menu
    fileMenuTree.push_back(SubTree(Icon(GNOME_STOCK_MENU_NEW),
							    N_("New"),
							    newMenuTree,
							    "Create a new project"));
  }

  {
    using namespace Gnome::MenuItems;
    fileMenuTree.push_back(Open(slot(this, &Project::openProject)));
    fileMenuTree.push_back(Save(slot(this, &Project::saveProject)));
    fileMenuTree.push_back(SaveAs(slot(this, &Project::saveAsProject)));

    fileMenuTree.push_back(Gnome::UI::Separator());
/*
    fileMenuTree.push_back(PrintSetup(slot(this, &Project::nothing_cb)));

    fileMenuTree.push_back(Gnome::UI::Item(Gnome::UI::Icon(GNOME_STOCK_MENU_PRINT),
								 N_("Print Cover..."),
								 slot(this, &Project::nothing_cb),
								 N_("Print Cover")));

    fileMenuTree.push_back(Gnome::UI::Separator());
*/
    // Close the current child (project);
    fileMenuTree.push_back(Close(slot(this, &Project::closeProject)));
    fileMenuTree.push_back(Exit(slot(gcdmaster, &GCDMaster::appClose)));
  }

  {
    using namespace Gnome::UI;
    // Actions menu
    actionsMenuTree.push_back(Item(Icon(GNOME_STOCK_MENU_CDROM),
								N_("_Record"),
								slot(this, &Project::recordToc2CD),
								N_("Record")));
    actionsMenuTree.push_back(Item(Icon(GNOME_STOCK_MENU_CDROM),
								N_("_CD to CD copy"),
								slot(gcdmaster, &GCDMaster::recordCD2CD),
								N_("CD to CD copy")));
    actionsMenuTree.push_back(Item(Icon(GNOME_STOCK_MENU_CDROM),
								N_("_Dump CD to disk"),
								slot(gcdmaster, &GCDMaster::recordCD2HD),
								N_("Dump CD to disk")));
/*
    actionsMenuTree.push_back(Gnome::UI::Item(N_("Fixate CD"),
					    slot(this, &Project::nothing_cb)));
    actionsMenuTree.push_back(Gnome::UI::Item(N_("Blank CD-RW"),
					    slot(this, &Project::nothing_cb)));
    actionsMenuTree.push_back(Gnome::UI::Item(N_("Get Info"),
					    slot(this, &Project::nothing_cb)));
*/
    // Settings menu
    settingsMenuTree.push_back(Item(Icon(GNOME_STOCK_MENU_PREF),
								N_("Configure Devices..."),
								slot(this, &Project::configureDevices)));
  }
/*
    settingsMenuTree.push_back(Gnome::MenuItems::Preferences
  				(slot(this, &Project::nothing_cb)));
*/

  // Help menu
  //helpMenuTree.push_back(Gnome::UI::Help("Quick Start"));

  helpMenuTree.push_back(Gnome::MenuItems::About
  				(slot(gcdmaster, &GCDMaster::aboutDialog)));

  {
    using namespace Gnome::Menus;
    menus.push_back(File(fileMenuTree));
    menus.push_back(Gnome::UI::Menu(N_("_Actions"), actionsMenuTree));
    menus.push_back(Settings(settingsMenuTree));
//    menus.push_back(Windows(windowsMenuTree));
    menus.push_back(Help(helpMenuTree));
  }

//FIXME  create_menus(menus);
  
  projectType = P_NONE;
  project_number = number;

  set_usize(350, 450);
}


void Project::newChooserWindow()
{
  if (projectType == P_CHOOSER)
    return;
  else if (projectType != P_NONE)
  {
    gcdmaster->newChooserWindow();
    return;
  }
  
  projectType = P_CHOOSER;
  projectChooser_ = new ProjectChooser;
  set_contents(*projectChooser_);
  projectChooser_->show();

  show();
}

void Project::newAudioCDProject()
{
  if (projectType == P_CHOOSER)
    delete (projectChooser_);
  else if (projectType != P_NONE)
  {
    gcdmaster->newAudioCDProject();
    return;
  }
  
  audioCDChild_ = new AudioCDChild(++project_number);
    
//FIXME  set_contents(*audioCDChild_);
  
//FIXME  guiUpdate();
}

void Project::openProject()
{
  if (readFileSelector_)
  {
    Gdk_Window selector_win = readFileSelector_->get_window();
    selector_win.show();
    selector_win.raise();
  }
  else
  {
    readFileSelector_ = new Gtk::FileSelection("Open project");
    readFileSelector_->get_ok_button()->clicked.connect(
				slot(this, &Project::readFileSelectorOKCB));
    readFileSelector_->get_cancel_button()->clicked.connect(
				slot(this, &Project::readFileSelectorCancelCB));
  }

  readFileSelector_->show();
}

void Project::readFileSelectorCancelCB()
{
  readFileSelector_->hide();
  readFileSelector_->destroy();
  readFileSelector_ = 0;
}

void Project::readFileSelectorOKCB()
{
  char *s = g_strdup(readFileSelector_->get_filename().c_str());

  if (s != NULL && *s != 0 && s[strlen(s) - 1] != '/') {
//FIXME: We should test what type of project it is
//       AudioCD, ISO. No problem now.
//FIXME    MDI_WINDOW->openAudioCDProject(s);
  }
  g_free(s);

  readFileSelectorCancelCB();
}

void Project::saveProject()
{
//  GenericChild *child = static_cast <GenericChild *>(this->get_active_child());

//  if (child)
    audioCDChild_->saveProject();
}

void Project::saveAsProject()
{
//  GenericChild *child = static_cast <GenericChild *>(this->get_active_child());

//  if (child)
    audioCDChild_->saveAsProject();
}

void Project::closeProject()
{
//  GenericChild *child = static_cast <GenericChild *>(this->get_active_child());

//  if (child)
    if (audioCDChild_->closeProject())
    {
//      remove(*child);
//      childs = g_list_remove(childs, child);
//      guiUpdate();
    }
}

void Project::recordToc2CD()
{
//  GenericChild *child = static_cast <GenericChild *>(this->get_active_child());

//  if (child)
    audioCDChild_->record_to_cd();
}

void Project::configureDevices()
{
  DEVICE_CONF_DIALOG->start();
}
