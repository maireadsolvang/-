import sqlite3
import datetime

# correlation_db = 'database.db'  # raw gesture data for correlation purposes
# game_db = 'game_database.db'  # contains game gestures table and game state table
correlation_db = '__HOME__/final_project/database.db'  # raw gesture data for correlation purposes
game_db = '__HOME__/final_project/game_database.db'  # contains game gestures table and game state table

# STARTING PLAYER HEALTH
health_1 = 5
health_2 = 5

now = datetime.datetime.now()
c = None  # filler for global variable
conn = None

ir_receive_threshold = now - datetime.timedelta(
    seconds=10)  # only gets action 6 (ir send) and starts a new game if within this threshold


def dict_factory(cursor, row):
    """
    Converts SQL rows to dictionary format.
    """
    d = {}
    for idx, col in enumerate(cursor.description):
        if col[0] == "timing":  # convert to datetime object
            d[col[0]] = datetime.datetime.strptime(row[idx], '%Y-%m-%d %H:%M:%S.%f')
        else:
            d[col[0]] = row[idx]
    return d


def request_handler(request):
    global c
    global conn
    global health_1
    global health_2
    ########## constants #############
    ## extracting data from post or get request##
    method = request["method"]
    # player action
    action = int(request["values"]["action"])
    # player ID
    player_ID = str(request["values"]["player_ID"])
    # game ID
    game_ID = int(request["values"]["game_ID"])

    ###################################

    ############## CONNECTING TO THE RIGHT DB ####################


    if action == 0:
        conn = sqlite3.connect(correlation_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()
    else:
        conn = sqlite3.connect(game_db)  # connect to that database (will create if it doesn't already exist)
        conn.row_factory = dict_factory
        c = conn.cursor()
    #############################################################

    if method == "POST":
        ## try to create table:
        try:
            if action == 0:
                c.execute(
                    '''CREATE TABLE raw_data_table (username text, timing timestamp, gestureID real, raw_data text);''')  # run a CREATE TABLE command
            else:
                # GESTURES TABLE
                c.execute(
                    '''CREATE TABLE gestures_table (game_ID integer, player_ID text, action integer, timing timestamp, checked integer);''')
                # GAME TABLE
                c.execute(
                    '''CREATE TABLE game_table (game_ID integer primary key autoincrement, player_1 text, health_1 integer, player_2 text, health_2 integer, game_state int);''')
                ## if already created, move to except
        except:
            pass

        if action == 0:  # CORRELATION
            ## get data from the body of the code #####
            form = request["form"]
            data = form["data"]
            username = form["username"]
            gestureID = int(form["gestureID"])
            c.execute('''INSERT into raw_data_table VALUES (?,?,?,?);''', (username, now, gestureID, data))

        else:
            ################# GET REQUEST ##############
            if action == 1:
                close_db()
                return "ERROR: this should be a get request!"
            ##############    Actions ##################
            elif action in {2, 3, 4}:  ## SLASH(2), PUSH(3) , BLOCK(4)
                ### updates gestures Table with new values ###
                add_action(game_ID, player_ID, action)

                ### get game information using game_ID ###
                game_info_dict = extract_game_info(game_ID)  # dictionary mapping item from game table

                ######### EXTRACTING INFO ##################
                game_state = game_info_dict["game_state"]
                player_1 = game_info_dict["player_1"]
                health_1 = game_info_dict["health_1"]
                player_2 = game_info_dict["player_2"]
                health_2 = game_info_dict["health_2"]

                if game_state != 1:
                    if player_ID == player_1:
                        close_db()
                        return str(health_1) + "," + str(game_state) + "," + str(game_ID)
                    elif player_ID == player_2:
                        close_db()
                        return str(health_2) + "," + str(game_state) + "," + str(game_ID)
                ############# END EXTRACTION #################

                ## extract relevant gestures from gestures table
                player_gestures = extract_player_gestures(player_1,
                                                          player_2)  ### creates a dictionary mapping player id to gesture
                action_1 = player_gestures[player_1]
                action_2 = player_gestures[player_2]

                ## calculate score
                score = health_update(health_1, health_2, action_1, action_2)
                health_1 = score[0]
                health_2 = score[1]

                ## change state = 2 (completed) when health of one person goes below 0
                if health_1 <= 0 or health_2 <= 0:
                    game_state = 2

                ## update the game table
                update_game_info(game_ID, health_1, health_2, game_state)

                if player_ID == player_1:
                    close_db()
                    return str(health_1) + "," + str(game_state) + "," + str(game_ID)
                elif player_ID == player_2:
                    close_db()
                    return str(health_2) + "," + str(game_state) + "," + str(game_ID)
            elif action == 5:  # CHARGE
                close_db()
                return "ERROR: Murd shut up: this never reaches the server"
            elif action == 7:  # BUTTON PRESS TO START GAME
                c.execute(
                    '''SELECT * FROM gestures_table WHERE (checked = 0 AND action = 7 AND player_ID != ? AND timing > ?);''',
                    (player_ID, ir_receive_threshold))
                results = c.fetchone()
                if results is None:  # if no BUTTON PRESS
                    add_action(-1, player_ID, action)
                    close_db()
                    return "-1,0,-1"
                else:
                    add_action(-1, player_ID, action, checked=1)
                    player_1 = results["player_ID"]
                    player_1_time = results["timing"]
                    ## player_1 is other Player
                    # player_2 is yourself
                    c.execute(
                        '''UPDATE gestures_table SET checked = 1 WHERE (player_ID = ? AND action = 7 AND timing = ?);''',
                        (player_1, player_1_time))  # set BUTTON PRESS checked to 1
                    c.execute('''INSERT into game_table VALUES (null,?,?,?,?,?);''',
                              (player_1, health_1, player_ID, health_2, 1))  # create new game
                    c.execute(
                        '''SELECT game_ID FROM game_table WHERE (player_1 = ? AND player_2 = ? AND game_state = 1);''',
                        (player_1, player_ID))
                    game_ID = c.fetchone()["game_ID"]
                    close_db()
                    return str(health_2) + "," + str(1) + "," + str(game_ID)

    elif method == "GET":
        #CHECKING TO SEE IF GAME HAS STARTED
        if action == 1:
            c.execute('''SELECT * FROM game_table WHERE (player_1 = ? AND game_state = 1);''', (player_ID,))
            results = c.fetchone()
            close_db()
            if results is None:
                return "-1,0,-1"
            return str(results['health_1']) + "," + str(results['game_state']) + "," + str(results['game_ID'])
        #GETTING UPDATED GAME STATS
        elif action == 8:
            game_info_dict = extract_game_info(game_ID)  # dictionary mapping item from game table
            ######### EXTRACTING INFO ##################
            if game_info_dict is None:
                return "-1,0,-1"

            else:
                game_state = game_info_dict["game_state"]
                player_1 = game_info_dict["player_1"]
                health_1 = game_info_dict["health_1"]
                player_2 = game_info_dict["player_2"]
                health_2 = game_info_dict["health_2"]
                if player_ID == player_1:
                    close_db()
                    return str(health_1) + "," + str(game_state) + "," + str(game_ID)
                elif player_ID == player_2:
                    close_db()
                    return str(health_2) + "," + str(game_state) + "," + str(game_ID)
                else:
                    return "Your player_ID is not in the current game"


def add_action(game_ID, player_ID, action_ID, time=now, checked=0):
    """
    Function that updates the gestures table with the corresponding inputs; addS gestures to the gesture table.
    Returns nothing.
    """
    c.execute('''INSERT into gestures_table VALUES (?,?,?,?,?)''', (game_ID, player_ID, action_ID, time, checked))


def update_game_info(game_ID, health_1, health_2, game_state):
    """
    Goes into the game table and will UPDATE (not make a new row) that row with these values.
    Returns game state.
    """
    c.execute('''UPDATE game_table SET health_1 = ?, health_2 = ?, game_state = ? WHERE game_ID = ?;''',
              (health_1, health_2, game_state, game_ID))
    return game_state


def extract_game_info(game_ID):
    """
    Takes in the game_ID and goes into the Game Table. 
    Returns the most recent row containing the game_ID 
    as a dictionary of the game information. 
    """
    c.execute('''SELECT * FROM game_table WHERE game_ID = ?;''', (game_ID,))
    results = c.fetchone()
    return results


def extract_player_gestures(player_1, player_2):
    """
    Looks at gesture database and finds the two most recent rows
    where one is player_1 and one is player_2. The timestamp of the rows
    must be within one second and checked for both must equal 0. If one 
    of the two rows has an action that is not block, change the check 
    value in that database row to 1. If they're not, one of the players 
    is None. Returns a dictionary of the player name and their action. 
    """
    recent_player_1 = c.execute('''SELECT * FROM gestures_table WHERE player_ID = ? ORDER BY timing DESC;''',
                                (player_1,)).fetchone()
    recent_player_2 = c.execute('''SELECT * FROM gestures_table WHERE player_ID = ? ORDER BY timing DESC;''',
                                (player_2,)).fetchone()
    time_dif = (recent_player_1['timing'] - recent_player_2['timing'])
    # times are greater than 1 sec apart and recent_player_1 was first
    if time_dif < datetime.timedelta(seconds=-1):
        recent_player_1 = None
    # times are greater than 1 sec apart and recent_player_2 was first
    elif time_dif > datetime.timedelta(seconds=1):
        recent_player_2 = None

    if recent_player_1 is not None:
        if recent_player_1['checked'] != 0:
            recent_player_1 = None

    if recent_player_2 is not None:
        if recent_player_2['checked'] != 0:
            recent_player_2 = None

    # If row has an action that is not block, change the check value to 1
    if recent_player_1 is not None and recent_player_1['action'] != 4:
        c.execute('''UPDATE gestures_table SET checked = 1 WHERE (player_ID = ? and timing = ?);''',
                  (player_1, recent_player_1['timing']))
    if recent_player_2 is not None and recent_player_2['action'] != 4:
        c.execute('''UPDATE gestures_table SET checked = 1 WHERE (player_ID = ? and timing = ?);''',
                  (player_2, recent_player_2['timing']))

    # Return dictionary of player name and their action
    if recent_player_1 is None and recent_player_2 is None:
        return {player_1: None, player_2: None}
    elif recent_player_1 is None:
        return {player_1: None, player_2: recent_player_2['action']}
    elif recent_player_2 is None:
        return {player_1: recent_player_1['action'], player_2: None}
    else:
        return {player_1: recent_player_1['action'], player_2: recent_player_2['action']}


def health_update(cur_health_1, cur_health_2, action_1, action_2):
    """
    Takes in the current healths of both players and the last action each of them made.
    Returns the updated healths of both players.
    """
    # Both None
    if action_1 == None and action_2 == None:
        return [cur_health_1, cur_health_2]
    # One slash and one None
    if action_1 == 2 and action_2 == None:
        return [cur_health_1, cur_health_2 - 1]
    if action_1 == None and action_2 == 2:
        return [cur_health_1 - 1, cur_health_2]
    # One push and one None
    if action_1 == 3 and action_2 == None:
        if cur_health_2 > 1:
            return [cur_health_1, cur_health_2 - 2]
        else:
            return [cur_health_1, 0]
    if action_1 == None and action_2 == 3:
        if cur_health_1 > 1:
            return [cur_health_1 - 2, cur_health_2]
        else:
            return [0, cur_health_2]
    # Any block
    if (action_1 == 3 and action_2 == 4):
        return [cur_health_1, cur_health_2 - 1]
    if (action_1 == 4 and action_2 == 3):
        return [cur_health_1 - 1, cur_health_2]
    if (action_1 == 4 or action_2 == 4):
        return [cur_health_1, cur_health_2]
    # one slash and one slash
    if action_1 == 2 and action_2 == 2:
        return [cur_health_1 - 1, cur_health_2 - 1]
    # one push and one push
    if action_1 == 3 and action_2 == 3:
        if cur_health_1 > 1 and cur_health_2 > 1:
            return [cur_health_1 - 2, cur_health_2 - 2]
        elif cur_health_1 > 1:
            return [cur_health_1 - 2, 0]
        elif cur_health_2 > 1:
            return [0, cur_health_2 - 2]
        else:
            return [0, 0]


def close_db():
    conn.commit()  # commit commands
    conn.close()  # close connection to database
