/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2011-2012, Digium, Inc.
 *
 * David Vossel <dvossel@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*!
 * \file
 * \brief Presence state unit tests
 */

/*** MODULEINFO
	<depend>TEST_FRAMEWORK</depend>
	<support_level>core</support_level>
 ***/

#include "asterisk.h"

#include "asterisk/test.h"
#include "asterisk/module.h"
#include "asterisk/channel.h"
#include "asterisk/presencestate.h"

#define TEST_CATEGORY "/main/presence/"

static int presence_test_alice_state = AST_PRESENCE_UNAVAILABLE;
static int presence_test_bob_state = AST_PRESENCE_UNAVAILABLE;

static int presence_test_presencestate(const char *label, char **subtype, char **message)
{
	if (!strcmp(label, "Alice")) {
		return presence_test_alice_state;
	} else if (!strcmp(label, "Bob")) {
		return presence_test_bob_state;
	} else {
		return AST_PRESENCE_UNAVAILABLE;
	}
}

static struct ast_channel_tech presence_test_tech = {
	.type = "PresenceTestChannel",
	.description = "Presence test technology",
	.presencestate = presence_test_presencestate,
};

AST_TEST_DEFINE(test_presence_chan)
{
	int res = AST_TEST_FAIL;
	char provider[80];
	enum ast_presence_state state;
	char *subtype = NULL, *message = NULL;

	switch (cmd) {
	case TEST_INIT:
		info->name = "channel_presence";
		info->category = TEST_CATEGORY;
		info->summary = "Channel presence state tests";
		info->description = "Creates test channel technology and then test the presence state callback";
		return AST_TEST_NOT_RUN;
	case TEST_EXECUTE:
		break;
	}

	if (ast_channel_register(&presence_test_tech)) {
		ast_log(LOG_WARNING, "Unable to register channel type '%s'\n", presence_test_tech.type);
		goto presence_test_cleanup;
	}

	/* Check Alice's state */
	snprintf(provider, sizeof(provider), "%s/Alice", presence_test_tech.type);

	presence_test_alice_state = AST_PRESENCE_AVAILABLE;
	state = ast_presence_state_nocache(provider, &subtype, &message);

	if (state != presence_test_alice_state) {
		ast_log(LOG_WARNING, "Presence state of '%s' returned '%s' instead of the expected value '%s'\n",
			provider, ast_presence_state2str(state), ast_presence_state2str(presence_test_alice_state));
		goto presence_test_cleanup;
	}

	/* Check Alice's and Bob's state, Alice's should win as DND > AVAILABLE */
	snprintf(provider, sizeof(provider), "%s/Alice&%s/Bob", presence_test_tech.type, presence_test_tech.type);

	presence_test_alice_state = AST_PRESENCE_DND;
	presence_test_bob_state = AST_PRESENCE_UNAVAILABLE;
	state = ast_presence_state_nocache(provider, &subtype, &message);

	if (state != presence_test_alice_state) {
		ast_log(LOG_WARNING, "Presence state of '%s' returned '%s' instead of the expected value '%s'\n",
			provider, ast_presence_state2str(state), ast_presence_state2str(presence_test_alice_state));
		goto presence_test_cleanup;
        }

	/* Check Alice's and Bob's state, Bob's should now win as AVAILABLE < UNAVAILABLE */
	presence_test_alice_state = AST_PRESENCE_AVAILABLE;
	state = ast_presence_state_nocache(provider, &subtype, &message);

	if (state != presence_test_bob_state) {
		ast_log(LOG_WARNING, "Presence state of '%s' returned '%s' instead of the expected value '%s'\n",
			provider, ast_presence_state2str(state), ast_presence_state2str(presence_test_bob_state));
		goto presence_test_cleanup;
	}

	res = AST_TEST_PASS;

presence_test_cleanup:
	ast_channel_unregister(&presence_test_tech);
	ast_free(subtype);
	ast_free(message);

	return res;
}

static int unload_module(void)
{
	AST_TEST_UNREGISTER(test_presence_chan);

	return 0;
}

static int load_module(void)
{
	AST_TEST_REGISTER(test_presence_chan);

	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Presence State test module");
