// player.cpp
//
// $Id: player.cpp,v 1.12 2003/07/23 03:16:36 sdennis Exp $
//

#include "copyright.h"
#include "autoconf.h"
#include "config.h"
#include "externs.h"

#include "attrs.h"
#include "comsys.h"
#include "functions.h"
#include "interface.h"
#include "powers.h"
#include "svdreport.h"
#include "sha1.h"

#define NUM_GOOD    4   // # of successful logins to save data for.
#define NUM_BAD     3   // # of failed logins to save data for.

typedef struct hostdtm HOSTDTM;
struct hostdtm
{
    char *host;
    char *dtm;
};

typedef struct logindata LDATA;
struct logindata
{
    HOSTDTM good[NUM_GOOD];
    HOSTDTM bad[NUM_BAD];
    int tot_good;
    int tot_bad;
    int new_bad;
};


/* ---------------------------------------------------------------------------
 * decrypt_logindata, encrypt_logindata: Decode and encode login info.
 */

static void decrypt_logindata(char *atrbuf, LDATA *info)
{
    int i;

    info->tot_good = 0;
    info->tot_bad = 0;
    info->new_bad = 0;
    for (i = 0; i < NUM_GOOD; i++)
    {
        info->good[i].host = NULL;
        info->good[i].dtm = NULL;
    }
    for (i = 0; i < NUM_BAD; i++)
    {
        info->bad[i].host = NULL;
        info->bad[i].dtm = NULL;
    }

    if (*atrbuf == '#')
    {
        atrbuf++;
        info->tot_good = mux_atol(grabto(&atrbuf, ';'));
        for (i = 0; i < NUM_GOOD; i++)
        {
            info->good[i].host = grabto(&atrbuf, ';');
            info->good[i].dtm = grabto(&atrbuf, ';');
        }
        info->new_bad = mux_atol(grabto(&atrbuf, ';'));
        info->tot_bad = mux_atol(grabto(&atrbuf, ';'));
        for (i = 0; i < NUM_BAD; i++)
        {
            info->bad[i].host = grabto(&atrbuf, ';');
            info->bad[i].dtm = grabto(&atrbuf, ';');
        }
    }
}

static void encrypt_logindata(char *atrbuf, LDATA *info)
{
    // Make sure the SPRINTF call tracks NUM_GOOD and NUM_BAD for the number
    // of host/dtm pairs of each type.
    //
    char nullc = '\0';
    int i;
    for (i = 0; i < NUM_GOOD; i++)
    {
        if (!info->good[i].host)
            info->good[i].host = &nullc;
        if (!info->good[i].dtm)
            info->good[i].dtm = &nullc;
    }
    for (i = 0; i < NUM_BAD; i++)
    {
        if (!info->bad[i].host)
            info->bad[i].host = &nullc;
        if (!info->bad[i].dtm)
            info->bad[i].dtm = &nullc;
    }
    char *bp = alloc_lbuf("encrypt_logindata");
    sprintf(bp, "#%d;%s;%s;%s;%s;%s;%s;%s;%s;%d;%d;%s;%s;%s;%s;%s;%s;",
        info->tot_good,
        info->good[0].host, info->good[0].dtm,
        info->good[1].host, info->good[1].dtm,
        info->good[2].host, info->good[2].dtm,
        info->good[3].host, info->good[3].dtm,
        info->new_bad, info->tot_bad,
        info->bad[0].host, info->bad[0].dtm,
        info->bad[1].host, info->bad[1].dtm,
        info->bad[2].host, info->bad[2].dtm);
    strcpy(atrbuf, bp);
    free_lbuf(bp);
}

/* ---------------------------------------------------------------------------
 * record_login: Record successful or failed login attempt.
 * If successful, report last successful login and number of failures since
 * last successful login.
 */

void record_login
(
    dbref player,
    bool  isgood,
    char  *ldate,
    char  *lhost,
    char  *lusername,
    char  *lipaddr
)
{
    LDATA login_info;
    dbref aowner;
    int aflags, i;

    char *atrbuf = atr_get(player, A_LOGINDATA, &aowner, &aflags);
    decrypt_logindata(atrbuf, &login_info);
    if (isgood)
    {
        if (login_info.new_bad > 0)
        {
            notify(player, "");
            notify(player, tprintf("**** %d failed connect%s since your last successful connect. ****",
                login_info.new_bad, (login_info.new_bad == 1 ? "" : "s")));
            notify(player, tprintf("Most recent attempt was from %s on %s.",
                login_info.bad[0].host, login_info.bad[0].dtm));
            notify(player, "");
            login_info.new_bad = 0;
        }
        if (  login_info.good[0].host
           && *login_info.good[0].host
           && login_info.good[0].dtm
           && *login_info.good[0].dtm)
        {
            notify(player, tprintf("Last connect was from %s on %s.",
                login_info.good[0].host, login_info.good[0].dtm));
        }
        if (mudconf.have_mailer)
        {
            check_mail(player, 0, false);
        }

        for (i = NUM_GOOD - 1; i > 0; i--)
        {
            login_info.good[i].dtm = login_info.good[i - 1].dtm;
            login_info.good[i].host = login_info.good[i - 1].host;
        }
        login_info.good[0].dtm = ldate;
        login_info.good[0].host = lhost;
        login_info.tot_good++;
        if (*lusername)
        {
            atr_add_raw(player, A_LASTSITE, tprintf("%s@%s", lusername, lhost));
        }
        else
        {
            atr_add_raw(player, A_LASTSITE, lhost);
        }

        // Add the players last IP too.
        //
        atr_add_raw(player, A_LASTIP, lipaddr);
    }
    else
    {
        for (i = NUM_BAD - 1; i > 0; i--)
        {
            login_info.bad[i].dtm = login_info.bad[i - 1].dtm;
            login_info.bad[i].host = login_info.bad[i - 1].host;
        }
        login_info.bad[0].dtm = ldate;
        login_info.bad[0].host = lhost;
        login_info.tot_bad++;
        login_info.new_bad++;
    }
    encrypt_logindata(atrbuf, &login_info);
    atr_add_raw(player, A_LOGINDATA, atrbuf);
    free_lbuf(atrbuf);
}

const char Base64Table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define ENCODED_LENGTH(x) ((((x)+2)/3)*4)

size_t EncodeBase64(size_t nIn, const char *pIn, char *pOut)
{
    int nTriples = nIn/3;
    int nLeftover = nIn%3;
    size_t nOut = 4 * nTriples;
    UINT8 ch0, ch1, ch2, ch3;

    while (nTriples--)
    {
        ch0 = ((UINT8)pIn[0]) >> 2;
        ch1 = ((((UINT8)pIn[0]) & 0x03) << 4)
            | (((UINT8)pIn[1]) >> 4);
        ch2 = ((((UINT8)pIn[1]) & 0x0F) << 2)
            | (((UINT8)pIn[2]) >> 6);
        ch3 = ((UINT8)pIn[2]) & 0x3F;

        pOut[0] = Base64Table[ch0];
        pOut[1] = Base64Table[ch1];
        pOut[2] = Base64Table[ch2];
        pOut[3] = Base64Table[ch3];

        pOut += 4;
        pIn  += 3;
    }
    
    switch (nLeftover)
    {
    case 1:
        ch0 = ((UINT8)pIn[0]) >> 2;
        ch1 = ((((UINT8)pIn[0]) & 0x03) << 4);
        pOut[0] = Base64Table[ch0];
        pOut[1] = Base64Table[ch1];
        pOut[2] = '=';
        pOut[3] = '=';
        nOut += 4;
        pOut += 4;
        break;

    case 2:
        ch0 = ((UINT8)pIn[0]) >> 2;
        ch1 = ((((UINT8)pIn[0]) & 0x03) << 4)
            | (((UINT8)pIn[1]) >> 4);
        ch2 = ((((UINT8)pIn[1]) & 0x0F) << 2);
        pOut[0] = Base64Table[ch0];
        pOut[1] = Base64Table[ch1];
        pOut[2] = Base64Table[ch2];
        pOut[3] = '=';
        nOut += 4;
        pOut += 4;
        break;
    }
    pOut[0] = '\0';
    return nOut;
}

#define SALT_LENGTH 9
#define ENCODED_SALT_LENGTH ENCODED_LENGTH(SALT_LENGTH)

const char *GenerateSalt(void)
{
    char szSaltRaw[SALT_LENGTH+1];
    int i;
    for (i = 0; i < SALT_LENGTH; i++)
    {
        szSaltRaw[i] = RandomINT32(0, 255);
    }
    szSaltRaw[SALT_LENGTH] = '\0';

    static char szSaltEncoded[ENCODED_SALT_LENGTH+1];
    EncodeBase64(SALT_LENGTH, szSaltRaw, szSaltEncoded);
    return szSaltEncoded;
}

void ChangePassword(dbref player, const char *szPassword)
{
    s_Pass(player, mux_crypt(szPassword, GenerateSalt()));
}

#define SHA1_PREFIX_LENGTH 6
const char szSHA1Prefix[SHA1_PREFIX_LENGTH+1] = "$SHA1$";
#define ENCODED_HASH_LENGTH ENCODED_LENGTH(5*sizeof(UINT32))

#define MD5_PREFIX_LENGTH 3
const char szMD5Prefix[MD5_PREFIX_LENGTH+1] = "$1$";
#define MD5_SALT_LENGTH  11


char *mux_crypt(const char *szPassword, const char *szSalt)
{
    if (ENCODED_SALT_LENGTH < strlen(szSalt))
    {
        // We should never get here as check_pass() validates
        // the length of the salt in the database, and
        // GenerateSalt() generates exactly the above length.
        //
        return "$FAIL$$";
    }

    // Calculate Hash.
    //
    SHA1_CONTEXT shac;

    SHA1_Init(&shac);
    SHA1_Compute(&shac, strlen(szSalt), szSalt);
    SHA1_Compute(&shac, strlen(szPassword), szPassword);
    SHA1_Final(&shac);

    // Serialize 5 UINT32 words into big-endian.
    //
    char szHashRaw[21];
    char *p = szHashRaw;

    int i;
    for (i = 0; i <= 4; i++)
    {
        *p++ = (UINT8)(shac.H[i] >> 24);
        *p++ = (UINT8)(shac.H[i] >> 16);
        *p++ = (UINT8)(shac.H[i] >>  8);
        *p++ = (UINT8)(shac.H[i]      );
    }
    *p = '\0';

    //          1         2         3         4
    // 12345678901234567890123456789012345678901234567
    // $SHA1$ssssssssssss$hhhhhhhhhhhhhhhhhhhhhhhhhhhh
    //
    static char buf[SHA1_PREFIX_LENGTH + ENCODED_SALT_LENGTH + 1 + ENCODED_HASH_LENGTH + 1 + 16];
    sprintf(buf, "%s%s$", szSHA1Prefix, szSalt);
    int n = strlen(buf);
    EncodeBase64(20, szHashRaw, buf + n);
    return buf;
}

/* ---------------------------------------------------------------------------
 * check_pass: Test a password to see if it is correct.
 */

bool check_pass(dbref player, const char *pPassword)
{
    bool  bValidPass = false;
    bool  bUpdatePass = false;

    int   aflags;
    dbref aowner;
    char *pTarget = atr_get(player, A_PASS, &aowner, &aflags);
    if (*pTarget)
    {
        size_t nTarget = strlen(pTarget);
        size_t nPassword = strlen(pPassword);
        if (  SHA1_PREFIX_LENGTH <= nTarget
           && memcmp(szSHA1Prefix, pTarget, SHA1_PREFIX_LENGTH) == 0)
        {
            // SHA-1 password.
            //
            char *pSalt = pTarget + SHA1_PREFIX_LENGTH;
            char *pHash;
            if (  *pSalt
               && (pHash = strchr(pSalt, '$')))
            {
                size_t nSalt = pHash - pSalt;
                pHash++;

                if (nSalt <= ENCODED_SALT_LENGTH)
                {
                    char szSalt[ENCODED_SALT_LENGTH+1];
                    memcpy(szSalt, pSalt, nSalt);
                    szSalt[nSalt] = '\0';

                    if (strcmp(mux_crypt(pPassword, szSalt), pTarget) == 0)
                    {
                        bValidPass = true;
                    }
                }
            }
        }
        else if (  MD5_PREFIX_LENGTH <= nTarget
                && memcmp(szMD5Prefix, pTarget, MD5_PREFIX_LENGTH) == 0)
        {
            char *pSalt = pTarget + MD5_PREFIX_LENGTH;
            char *pHash;
            if (  *pSalt
               && (pHash = strchr(pSalt, '$')))
            {
                size_t nSalt = pHash - pTarget;
                pHash++;

                if (nSalt <= MD5_SALT_LENGTH)
                {
                    char szSalt[MD5_SALT_LENGTH+1];
                    memcpy(szSalt, pTarget, nSalt);
                    szSalt[nSalt] = '\0';

                    if (strcmp(crypt(pPassword, szSalt), pTarget) == 0)
                    {
                        bValidPass = true;
                        bUpdatePass = true;
                    }
                }
            }
        }
        else if (  nTarget == 13
                && memcmp("XX", pTarget, 2) == 0)
        {
            // DES
            //
            if (strcmp(crypt(pPassword, "XX"), pTarget) == 0)
            {
                bValidPass = true;
                bUpdatePass = true;
            }
        }
        else
        {
            // Clear-text password.
            //
            if (strcmp(pTarget, pPassword) == 0)
            {
                bValidPass = true;
                bUpdatePass = true;
            }
        }
    }

    if (bUpdatePass)
    {
        // Upgrade password to SHA-1.
        //
        ChangePassword(player, pPassword);
    }
    free_lbuf(pTarget);
    return bValidPass;
}

/* ---------------------------------------------------------------------------
 * connect_player: Try to connect to an existing player.
 */

dbref connect_player(char *name, char *password, char *host, char *username, char *ipaddr)
{
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetLocal();
    char *time_str = ltaNow.ReturnDateString(7);

    dbref player = lookup_player(NOTHING, name, false);
    if (player == NOTHING)
    {
        return NOTHING;
    }
    if (!check_pass(player, password))
    {
        record_login(player, false, time_str, host, username, ipaddr);
        return NOTHING;
    }

    // Compare to last connect see if player gets salary.
    //
    int aflags;
    dbref aowner;
    char *player_last = atr_get(player, A_LAST, &aowner, &aflags);
    if (strncmp(player_last, time_str, 10) != 0)
    {
        char *allowance = atr_pget(player, A_ALLOWANCE, &aowner, &aflags);
        if (*allowance == '\0')
        {
            giveto(player, mudconf.paycheck);
        }
        else
        {
            giveto(player, mux_atol(allowance));
        }
        free_lbuf(allowance);
    }
    free_lbuf(player_last);
    atr_add_raw(player, A_LAST, time_str);
    return player;
}

/* ---------------------------------------------------------------------------
 * create_player: Create a new player.
 */

dbref create_player(char *name, char *password, dbref creator, bool isrobot, bool isguest)
{
    // Make sure the password is OK.  Name is checked in create_obj.
    //
    char *pbuf = trim_spaces(password);
    if (!ok_password(pbuf, creator))
    {
        free_lbuf(pbuf);
        return NOTHING;
    }

    // If so, go create him.
    //
    dbref player = create_obj(creator, TYPE_PLAYER, name, isrobot);
    if (player == NOTHING)
    {
        free_lbuf(pbuf);
        return NOTHING;
    }

    // Initialize everything.
    //
    if (isguest)
    {
        if (*mudconf.guests_channel)
        {
            do_addcom(player, player, player, 0, 2, "g", mudconf.guests_channel);
        }
    }
    else
    {
        if (*mudconf.public_channel)
        {
            do_addcom(player, player, player, 0, 2, "pub", mudconf.public_channel);
        }
    }

    ChangePassword(player, pbuf);
    s_Home(player, start_home());
    free_lbuf(pbuf);
    return player;
}

/* ---------------------------------------------------------------------------
 * do_password: Change the password for a player
 */

void do_password
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   key,
    int   nargs,
    char *oldpass,
    char *newpass
)
{
    dbref aowner;
    int   aflags;
    char *target = atr_get(executor, A_PASS, &aowner, &aflags);
    if (  !*target
       || !check_pass(executor, oldpass))
    {
        notify(executor, "Sorry.");
    }
    else if (ok_password(newpass, executor))
    {
        atr_add_raw(executor, A_PASS, crypt(newpass, "XX"));
        notify(executor, "Password changed.");
    }
    free_lbuf(target);
}

/* ---------------------------------------------------------------------------
 * do_last: Display login history data.
 */

static void disp_from_on(dbref player, char *dtm_str, char *host_str)
{
    if (dtm_str && *dtm_str && host_str && *host_str)
    {
        notify(player,
               tprintf("     From: %s   On: %s", dtm_str, host_str));
    }
}

void do_last(dbref executor, dbref caller, dbref enactor, int key, char *who)
{
    dbref target, aowner;
    int i, aflags;

    if (  !who
       || !*who)
    {
        target = Owner(executor);
    }
    else if (string_compare(who, "me") == 0)
    {
        target = Owner(executor);
    }
    else
    {
        target = lookup_player(executor, who, true);
    }

    if (target == NOTHING)
    {
        notify(executor, "I couldn't find that player.");
    }
    else if (!Controls(executor, target))
    {
        notify(executor, NOPERM_MESSAGE);
    }
    else
    {
        char *atrbuf = atr_get(target, A_LOGINDATA, &aowner, &aflags);
        LDATA login_info;
        decrypt_logindata(atrbuf, &login_info);

        notify(executor, tprintf("Total successful connects: %d", login_info.tot_good));
        for (i = 0; i < NUM_GOOD; i++)
        {
            disp_from_on(executor, login_info.good[i].host, login_info.good[i].dtm);
        }
        notify(executor, tprintf("Total failed connects: %d", login_info.tot_bad));
        for (i = 0; i < NUM_BAD; i++)
        {
            disp_from_on(executor, login_info.bad[i].host, login_info.bad[i].dtm);
        }
        free_lbuf(atrbuf);
    }
}

/* ---------------------------------------------------------------------------
 * add_player_name, delete_player_name, lookup_player:
 * Manage playername->dbref mapping
 */

bool add_player_name(dbref player, const char *name)
{
    bool stat;
    char *temp, *tp;

    // Convert to all lowercase.
    //
    tp = temp = alloc_lbuf("add_player_name");
    safe_str(name, temp, &tp);
    *tp = '\0';
    mux_strlwr(temp);

    dbref *p = (int *)hashfindLEN(temp, strlen(temp), &mudstate.player_htab);
    if (p)
    {
        // Entry found in the hashtable.  If a player, succeed if the
        // numbers match (already correctly in the hash table), fail
        // if they don't. Fail if the name is a disallowed name
        // (value AMBIGUOUS).
        //
        if (*p == AMBIGUOUS)
        {
            free_lbuf(temp);
            return false;
        }
        if (Good_obj(*p) && isPlayer(*p))
        {
            free_lbuf(temp);
            if (*p == player)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // It's an alias (or an incorrect entry). Clobber it.
        //
        MEMFREE(p);
        p = (dbref *)MEMALLOC(sizeof(int));
        (void)ISOUTOFMEMORY(p);

        *p = player;
        stat = hashreplLEN(temp, strlen(temp), p, &mudstate.player_htab);
        free_lbuf(temp);
    }
    else
    {
        p = (dbref *)MEMALLOC(sizeof(int));
        (void)ISOUTOFMEMORY(p);

        *p = player;
        stat = (hashaddLEN(temp, strlen(temp), p, &mudstate.player_htab) >= 0);
        free_lbuf(temp);
    }
    return stat;
}

bool delete_player_name(dbref player, const char *name)
{
    char *temp, *tp;

    tp = temp = alloc_lbuf("delete_player_name");
    safe_str(name, temp, &tp);
    *tp = '\0';
    mux_strlwr(temp);

    dbref *p = (int *)hashfindLEN(temp, strlen(temp), &mudstate.player_htab);
    if (  !p
       || *p == NOTHING
       || (  player != NOTHING
          && *p != player))
    {
        free_lbuf(temp);
        return false;
    }
    MEMFREE(p);
    p = NULL;
    hashdeleteLEN(temp, strlen(temp), &mudstate.player_htab);
    free_lbuf(temp);
    return true;
}

dbref lookup_player(dbref doer, char *name, bool check_who)
{
    if (string_compare(name, "me") == 0)
    {
        return doer;
    }

    while (*name == LOOKUP_TOKEN)
    {
        name++;
    }
    dbref thing;
    if (*name == NUMBER_TOKEN)
    {
        name++;
        if (!is_integer(name, NULL))
        {
            return NOTHING;
        }
        thing = mux_atol(name);
        if (!Good_obj(thing))
        {
            return NOTHING;
        }
        if ( !(  isPlayer(thing)
              || God(doer)))
        {
            thing = NOTHING;
        }
        return thing;
    }
    char *temp, *tp;
    tp = temp = alloc_lbuf("lookup_player");
    safe_str(name, temp, &tp);
    *tp = '\0';
    mux_strlwr(temp);
    dbref *p = (int *)hashfindLEN(temp, strlen(temp), &mudstate.player_htab);
    free_lbuf(temp);
    if (!p)
    {
        if (check_who)
        {
            thing = find_connected_name(doer, name);
            if (Hidden(thing))
            {
                thing = NOTHING;
            }
        }
        else
        {
            thing = NOTHING;
        }
    }
    else if (!Good_obj(*p))
    {
        thing = NOTHING;
    }
    else
    {
        thing = *p;
    }

    return thing;
}

void load_player_names(void)
{
    dbref i;
    DO_WHOLE_DB(i)
    {
        if (isPlayer(i))
        {
            add_player_name(i, Name(i));
        }
    }
    char *alias = alloc_lbuf("load_player_names");
    DO_WHOLE_DB(i)
    {
        if (isPlayer(i))
        {
            dbref aowner;
            int aflags;
            alias = atr_pget_str(alias, i, A_ALIAS, &aowner, &aflags);
            if (*alias)
            {
                add_player_name(i, alias);
            }
        }
    }
    free_lbuf(alias);
}

/* ---------------------------------------------------------------------------
 * badname_add, badname_check, badname_list: Add/look for/display bad names.
 */

void badname_add(char *bad_name)
{
    // Make a new node and link it in at the top.
    //
    BADNAME *bp = (BADNAME *)MEMALLOC(sizeof(BADNAME));
    (void)ISOUTOFMEMORY(bp);
    bp->name = StringClone(bad_name);
    bp->next = mudstate.badname_head;
    mudstate.badname_head = bp;
}

void badname_remove(char *bad_name)
{
    // Look for an exact match on the bad name and remove if found.
    //
    BADNAME *bp;
    BADNAME *backp = NULL;
    for (bp = mudstate.badname_head; bp; backp = bp, bp = bp->next)
    {
        if (!string_compare(bad_name, bp->name))
        {
            if (backp)
            {
                backp->next = bp->next;
            }
            else
            {
                mudstate.badname_head = bp->next;
            }
            MEMFREE(bp->name);
            bp->name = NULL;
            MEMFREE(bp);
            bp = NULL;
            return;
        }
    }
}

bool badname_check(char *bad_name)
{
    BADNAME *bp;

    // Walk the badname list, doing wildcard matching.  If we get a hit then
    // return false.  If no matches in the list, return true.
    //
    for (bp = mudstate.badname_head; bp; bp = bp->next)
    {
        mudstate.wild_invk_ctr = 0;
        if (quick_wild(bp->name, bad_name))
        {
            return false;
        }
    }
    return true;
}

void badname_list(dbref player, const char *prefix)
{
    BADNAME *bp;
    char *buff, *bufp;

    // Construct an lbuf with all the names separated by spaces.
    //
    buff = bufp = alloc_lbuf("badname_list");
    safe_str(prefix, buff, &bufp);
    for (bp = mudstate.badname_head; bp; bp = bp->next)
    {
        safe_chr(' ', buff, &bufp);
        safe_str(bp->name, buff, &bufp);
    }
    *bufp = '\0';

    // Now display it.
    //
    notify(player, buff);
    free_lbuf(buff);
}
