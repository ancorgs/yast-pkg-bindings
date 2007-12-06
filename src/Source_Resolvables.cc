/* ------------------------------------------------------------------------------
 * Copyright (c) 2007 Novell, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may find
 * current contact information at www.novell.com.
 * ------------------------------------------------------------------------------
 */

/*
   File:	$Id:$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions for adding/removing resolvables in the pool
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

/*
 * A helper function - remove all resolvables from the repository from the pool
 */
void PkgModuleFunctions::RemoveResolvablesFrom(const std::string &alias)
{
    // remove the resolvables if they have been loaded
    // FIXME: can be implemented better? we need a ResStore object for removing,
    // which means search for a resolvable in the pool and get the Repository
    // object which can be asked for all resolvables in it
    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	; it != zypp_ptr()->pool().end()
	; ++it)
    {
	if (it->resolvable()->repository().info().alias() == alias)
	{
	    y2milestone("Removing all resolvables from '%s' from the pool...", alias.c_str());
	    zypp_ptr()->removeResolvables(it->resolvable()->repository().resolvables());
	    break;
	}
    }
}

/*
 * A helper function - is there any resolvable from the repository in the pool?
 */
bool PkgModuleFunctions::AnyResolvableFrom(const std::string &alias)
{
    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	; it != zypp_ptr()->pool().end()
	; ++it)
    {
	if (it->resolvable()->repository().info().alias() == alias)
	{
	    return true;
	}
    }

    return false;
}

/*
 * A helper function - load resolvable from the repository into the pool
 * Warning: Use AnyResolvableFrom() method for checing if the resolvables might be already loaded
 */
bool PkgModuleFunctions::LoadResolvablesFrom(const zypp::RepoInfo &repoinfo)
{
    bool success = true;
    unsigned int size_start = zypp_ptr()->pool().size();
    y2milestone("Loading resolvables from '%s', pool size at start: %d", repoinfo.alias().c_str(), size_start);

    try 
    {
	zypp::RepoManager repomanager = CreateRepoManager();

	// build cache if needed
	if (!repomanager.isCached(repoinfo))
	{
	    zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus(repoinfo);
	    if (raw_metadata_status.empty())
	    {
		y2milestone("Missing metadata for source '%s', downloading...", repoinfo.alias().c_str());
		RefreshWithCallbacks(repoinfo);
	    }

	    y2milestone("Caching source '%s'...", repoinfo.alias().c_str());
	    repomanager.buildCache(repoinfo);
	}

	zypp::Repository repository = repomanager.createFromCache(repoinfo);
	const zypp::ResStore &store = repository.resolvables();

	// load resolvables
	zypp_ptr()->addResolvables(store);

	y2milestone("Loaded %zd resolvables", store.size());
    }
    catch(const zypp::repo::RepoNotCachedException &excpt )
    {
	std::string alias = repoinfo.alias();
	y2error ("Resolvables from '%s' havn't been loaded: %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError("'" + alias + "': " + ExceptionAsString(excpt));
	success = false;

	// FIXME ??
	/*
	// disable the source
	y2error("Disabling source %s", url.c_str());
	repo->disable();
	*/
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repoinfo.alias();
	y2internal("Error: Loading resolvables failed: %s", ExceptionAsString(excpt).c_str());
	_last_error.setLastError("'" + alias + "': " + ExceptionAsString(excpt));
	success = false;
    }

    unsigned int size_end = zypp_ptr()->pool().size();
    y2milestone("Pool size at end: %d (loaded %d resolvables)", size_end, size_end - size_start);
    return success;
}