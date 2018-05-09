import sqlite3
import datetime

db = '__HOME__/final_project/database.db'
game_db = '__HOME__/final_project/game_database.db'

# action
# 1: creating new game
# 2: sending gesture data
#

def analyze_gesture(data):
    return 0
    #return gesture ID corresponding to data -- -1 if none

def request_handler(request):

    now = datetime.datetime.now()
    if request["method"] == "POST": #puts encrypted response in database
        form = request["form"]
        try:
            action = int(form["action"])
            if action == 0:
                data = form["data"]
                username = form["username"]
                gestureID = int(form["gestureID"])

                conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
                c = conn.cursor()  # make cursor into database (allows us to execute commands)
            elif action == 1:
                user1 = form["user1"]
                user2 = form["user2"]

                conn = sqlite3.connect(game_db)  # connect to that database (will create if it doesn't already exist)
                c = conn.cursor()  # make cursor into database (allows us to execute commands)
            elif action == 2:
                user = form["user"]
                data = form["data"]

                conn = sqlite3.connect(game_db)  # connect to that database (will create if it doesn't already exist)
                c = conn.cursor()  # make cursor into database (allows us to execute commands)

        except Exception:
            return "-1"

        try:
            if action == 0:
                c.execute('''CREATE TABLE raw_data_table (username text, timing timestamp, gestureID real, raw_data text);''')  # run a CREATE TABLE command
            elif action == 1:
                c.execute('''CREATE TABLE game_stats_table (gameID integer primary key autoincrement, user1 text, user2 text, user1Score integer, user2Score integer, game_status text, start_time timestamp);''')  # run a CREATE TABLE command
            elif action == 2:
                c.execute('''CREATE TABLE game_gestures_table (gameID integer primary key, user, gestureID real, time timestamp);''')  # run a CREATE TABLE command
        except:
            pass

        if action == 0:
            c.execute('''INSERT into raw_data_table VALUES (?,?,?,?);''', (username, now, gestureID, data))
        elif action == 1:
            c.execute('''INSERT into game_stats_table VALUES (?,?,?,?,?,?);''', (user1, user2, 0, 0, 'ongoing', now))
        elif action == 2:
            gestureID = analyze_gesture(data)
            gameID = c.execute('''SELECT gameID FROM game_stats_table WHERE game_status = 'ongoing' AND (user1 = ? OR user2 = ?);''', (user,)).fetchone()[0] #check if returns right data type
            c.execute('''INSERT into game_gestures_table VALUES (?,?,?,?);''', (gameID, user, gestureID, now))
        conn.commit()  # commit commands
        conn.close()  # close connection to database
        return data

    elif request['method'] == 'GET': #is a GET request

        args = request["args"]
        if 'table' not in args:
            return '-1'

        try:
            table = int(request["values"]['table'])

        except Exception:
            return '-1'

        all = []
        #twenty_seconds_ago = timestamp - datetime.timedelta(seconds=20)  # create time for 20 seconds ago!
        things = c.execute('''SELECT * FROM ?;''', (table,)).fetchall()
        for x in things:
            all.append(x[0])

        return str(all)

def health_update(cur_health_1,cur_health_2,action_1,action_2):
    """
    Takes in the current healths of both players and the last action each of them made.
    Returns the updated healths of both players.
    """
    #Both None
    if action_1 == None and action_2 == None:
        return [cur_health_1,cur_health_2]
    #One slash and one None
    if action_1 == 2 and action_2 == None:
        return [cur_health_1,cur_health_2-1]
    if action_1 == None and action_2 == 2:
        return [cur_health_1-1,cur_health_2]
    #One push and one None
    if action_1 == 3 and action_2 == None:
        if cur_health_2 > 1:
            return [cur_health_1,cur_health_2-2]
        else:
            return [cur_health_1,0]
    if action_1 == None and action_2 == 3:
        if cur_health_1 > 1:
            return [cur_health_1-2,cur_health_2]
        else:
            return [0,cur_health_2]
    #Any block
    if (action_1 == 2 and action_2 == 4) or (action_1 == 4 and action_2 == 2) or (action_1 == 3 and action_2 == 4) or (action_1 == 4 and action_2 == 3):
        return [cur_health_1,cur_health_2]
    #one slash and one slash
    if action_1 == 2 and action_2 == 2:
        return [cur_health_1,cur_health_2]
    #one push and one push
    if action_1 == 3 and action_2 == 3:
        if cur_health_1 > 1 and cur_health_2 > 1:
            return [cur_health_1-2,cur_health_2-2]
        elif cur_health_1 > 1:
            return [cur_health_1-2,0]
        elif cur_health_2 > 1:
            return [0,cur_health_2-2]
        else:
            return [0,0]


