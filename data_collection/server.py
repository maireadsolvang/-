import sqlite3
import datetime

db = '__HOME__/final_project/database.db'

def request_handler(request):
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # make cursor into database (allows us to execute commands)
    now = datetime.datetime.now()
    if request["method"] == "POST": #puts encrypted response in database
        form = request["form"]
        try:
            data = form["data"]
            username = form["username"]
            gestureID = int(form["gestureID"])

        except Exception:
            return "-1"

        try:
            c.execute('''CREATE TABLE raw_data_table (username text, timing timestamp, gestureID real, raw_data text);''')  # run a CREATE TABLE command
        except:
            pass

        c.execute('''INSERT into raw_data_table VALUES (?,?,?,?);''', (username, now, gestureID, data))

        conn.commit()  # commit commands
        conn.close()  # close connection to database
        return data

    elif request['method'] == 'GET': #is a GET request

        args = request["args"]
        if 'gestureID' not in args:
            return '-1'

        try:
            gestureID = int(request["values"]['gestureID'])

        except Exception:
            return '-1'

        all = []
        #twenty_seconds_ago = timestamp - datetime.timedelta(seconds=20)  # create time for 20 seconds ago!
        things = c.execute('''SELECT raw_data FROM raw_data_table WHERE gestureID = ?;''', (gestureID,)).fetchall()
        for x in things:
            all.append(x[0])

        return str(all)



