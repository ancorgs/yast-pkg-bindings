/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:	PkgModuleFunctionsSelection.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Package Selection Manager
   Namespace:   Pkg

   Purpose:	Access to PMSelectionManager
		Handles selection related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/

#include <fstream>

#include <ycp/y2log.h>
#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/ResPool.h>
#include <zypp/ResTraits.h>
#include <zypp/Resolvable.h>
#include <zypp/Selection.h>

using std::string;



// ------------------------
/**
   @builtin GetSelections

   @short Return list of selections matching given status
   @description
   returns a list of selection names matching the status symbol
     and the category.

   If category == "base", base selections are returned

   If category == "", addon selections are returned
   else selections matching the given category are returned

   status can be:
   <code>
   `all		: all known selections<br>
   `available	: available selections<br>
   `selected	: selected but not yet installed selections<br>
   `installed	: installed selection<br>
   </code>
   @param symbol status `all,`selected or `installed
   @param string category base or empty string for addons
   @return list<string>

*/
YCPValue
PkgModuleFunctions::GetSelections (const YCPSymbol& stat, const YCPString& cat)
{
    string status = stat->symbol();
    string category = cat->value();

    // FIXME: this should be done more nicely
    if ( category == "base" ) 
	category = "baseconf";

    YCPList selections;

    for (zypp::ResPool::byKind_iterator it 
	= zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Selection>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Selection>::kind) ; ++it )
    {
	string selection;

	if (status == "all" || status == "available")
	{
	    selection = it->resolvable()->name();
	}
	else if (status == "selected")
	{
	    if( it->status().isToBeInstalled() )
	        selection = it->resolvable()->name();
	}
	else if (status == "installed")
	{
	    if( it->status().wasInstalled() )
	        selection = it->resolvable()->name();
	}
	else
	{
	    y2warning (string ("Unknown status in Pkg::GetSelections(" + status + ", ...)").c_str());
	    break;
	}

	if (selection.empty())
	{
	    continue;
	}

	string selection_cat = zypp::dynamic_pointer_cast<const zypp::Selection>(it->resolvable())->category();
	if (category != "")
	{
	    if ( selection_cat != category)
	    {
		continue;			// asked for explicit category
	    }
	}
	else	// category == ""
	{
	    if (selection_cat == "baseconf")
	    {
		continue;			// asked for non-base
	    }
	}
	selections->add (YCPString (selection));
    }
    return selections;
}



// ------------------------
/**
   @builtin SelectionData

   @short Get Selection Data
   @description

   Return information about selection

   <code>
  	->	$["summary" : "This is a nice selection",
  		"category" : "Network",
  		"visible" : true,
  		"recommends" : ["sel1", "sel2", ...],
  		"suggests" : ["sel1", "sel2", ...],
  		"archivesize" : 12345678
  		"order" : "042",
		"requires" : ["a", "b"],
		"conflicts" : ["c"],
		"provides" : ["d"],
		"obsoletes" : ["e", "f"],
		]
   </code>

   Get summary (aka label), category, visible, recommends, suggests, archivesize,
   order attributes of a selection, requires, conflicts, provides and obsoletes.

   @param string selection
   @return map Returns an empty map if no selection found and
   Returns nil if called with wrong arguments

*/

YCPValue
PkgModuleFunctions::SelectionData (const YCPString& sel)
{
    YCPMap data;
    string name = sel->value();

    /* TODO FIXME

    data->add (YCPString ("archivesize"), YCPInteger ((long long) (selection->archivesize())));

    tiny_helper_no1 (&data, "requires", selection->requires ());
    tiny_helper_no1 (&data, "conflicts", selection->conflicts ());
    tiny_helper_no1 (&data, "provides", selection->provides ());
    tiny_helper_no1 (&data, "obsoletes", selection->obsoletes ());
    */

   zypp::ResPool::byName_iterator it = std::find_if (
        zypp_ptr->pool().byNameBegin(name)
        , zypp_ptr->pool().byNameEnd(name)
        , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Selection>::kind)
    );

    if ( it != zypp_ptr->pool().byNameEnd(name) )
    {
	zypp::Selection::constPtr selection = 
	    zypp::dynamic_pointer_cast<const zypp::Selection>(it->resolvable ());

	// selection found
	data->add (YCPString ("summary"), YCPString (selection->summary()));
	data->add (YCPString ("category"), YCPString (selection->category()));
	data->add (YCPString ("visible"), YCPBoolean (selection->visible()));
	data->add (YCPString ("order"), YCPString (selection->order()));
	data->add (YCPString ("description"), YCPString (selection->description()));

/*	YCPList recommendslist;
	std::set<std::string> recommends = selection->recommends();

	for (std::set<std::string>::const_iterator rec = recommends.begin();
	    rec != recommends.end(); rec++)
	{
	    if (!((*rec).empty()))
	    {
		recommendslist->add(YCPString(*rec));
	    }
	}
	data->add (YCPString("recommends"), recommendslist);

	YCPList suggestslist;
	std::set<std::string> suggests = selection->suggests();

	for (std::set<std::string>::const_iterator sug = suggests.begin();
	    sug != suggests.end(); sug++)
	{
	    if (!((*sug).empty()))
	    {
		suggestslist->add(YCPString(*sug));
	    }
	}
	data->add (YCPString("suggests"), recommendslist);
*/
#warning Report also archivesize, requires, provides, conflicts and obsoletes
    }
    else
    {
	return YCPError ("Selection '" + name + "' not found", data);
    }

    return data;
}


// ------------------------
/**
   @builtin SelectionContent
   @short Get list of packages listed in a selection
   @param string selection name of selection
   @param boolean to_delete if false, return packages to be installed
   if true, return packages to be deleted
   @param string language if "" (empty), return only non-language specific packages
   else return only packages machting the language
   @return list Returns an empty list if no matching selection found
   Returns nil if called with wrong arguments

*/

YCPValue
PkgModuleFunctions::SelectionContent (const YCPString& sel, const YCPBoolean& to_delete, const YCPString& lang)
{
#warning Pkg::SelectionContent to_delete is not supported

    YCPList data;
    std::string name = sel->value();
    std::string locale = lang->value();
    
    zypp::ResPool::byName_iterator it = std::find_if (
        zypp_ptr->pool().byNameBegin(name)
        , zypp_ptr->pool().byNameEnd(name)
        , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Selection>::kind)
    );

    if ( it != zypp_ptr->pool().byNameEnd(name) )
    {
	zypp::Selection::constPtr selection = 
	    zypp::dynamic_pointer_cast<const zypp::Selection>(it->resolvable ());

#warning implemented correctly?
	// is it correct?
	std::set<std::string> inst = selection->install_packages(zypp::Locale(locale));

	for (std::set<std::string>::const_iterator inst_it = inst.begin();
	    inst_it != inst.end(); inst_it++)
	{
	    if (!((*inst_it).empty()))
	    {
		data->add(YCPString(*inst_it));
	    }
	}
    }
    else
    {
	return YCPError ("Selection '" + name + "' not found", data);
    }

    return data;
}


// ------------------------


/**
   @builtin SetSelection

   @short Set a new selection
   @description
   If the selection is a base selection,
   this effetively resets the current package selection to
   the packages of the newly selected base selection
   Usually returns true
   @param string selection
   @return boolean Returns false if the given string does not match
   a known selection.

*/
YCPBoolean
PkgModuleFunctions::SetSelection (const YCPString& selection)
{
    return DoProvideNameKind( selection->value(), zypp::ResTraits<zypp::Selection>::kind);
}

// ------------------------
/**
   @builtin ClearSelection

   @short Clear a selected selection
   @param string selection
   @return boolean


*/
YCPValue
PkgModuleFunctions::ClearSelection (const YCPString& selection)
{
    ycpwarning( "Pkg::ClearSelection does not reset add-on selections anymore");
    return YCPBoolean( DoRemoveNameKind( selection->value(), zypp::ResTraits<zypp::Selection>::kind ) );
}


// ------------------------
/**
   @builtin ActivateSelections

   @short Activate all selected selections
   @description
   To be called when user is done with selections and wants
   to continue on the package level (or finish)

   This will transfer the selection status to package status

   @return boolean
   
   @deprecated Use Pkg::PkgSolve instead, selections are solvable now
*/
YCPBoolean
PkgModuleFunctions::ActivateSelections ()
{
    y2warning ("Pkg::ActivateSelections is obsolete. Use Pkg::PkgSolve instead");

    return YCPBoolean (true);
}

