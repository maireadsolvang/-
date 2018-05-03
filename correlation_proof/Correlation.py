import math
import sqlite3
import re
import numpy as np

db = '__HOME__/final_project/database.db'


def request_handler(request):
    print('Data 2 \n')
    print(d2)
    print('Data 3 \n')
    print(d3)
    # return d2, d3

def linear(samples, t, sample_rate=20):
    #samples: list of discreet samples
    #t: time in sec
    #sample_rate: in hz, defaults to equiv of 50ms
    i1 = math.floor(sample_rate*t)
    i2 = math.ceil(sample_rate*t)
    y1 = float(samples[i1])
    try: y2 = float(samples[i2])
    #take into account python arithmatic errors
    except: y2 = float(samples[len(samples)-1])
    x1 = i1/sample_rate
    x2 = i2/sample_rate
    #if sample_rate*t is already a defined value
    if i1 == i2:
        return samples[i1]
    return (y2-y1)/(x2-x1)*(t-x1) + y1


def scale(samples, new_length, interpolator, sample_rate=20):
    #samples: list of discreet samples
    #new_length: desired length of new list
    #interpolator: function to interpolate values
    #sample_rate: in hz

    #convert index of desired list to time
    conversion = ((len(samples)-1)/(new_length-1))/sample_rate
    l = []
    x = 0
    for x in range(new_length):
        l.append(interpolator(samples, x*conversion, sample_rate))
    return l

l = []
conn = sqlite3.connect(db)  # connect to that database
c = conn.cursor()  # make cursor into database (allows us to execute commands)
data2 = c.execute('''SELECT * FROM raw_data_table WHERE abs(gestureID - 2) < 0.1;''').fetchall()
data3 = c.execute('''SELECT * FROM raw_data_table WHERE abs(gestureID - 3) < 0.1;''').fetchall()
conn.commit() # commit commands
conn.close() # close connection to database



def get_corr(data):
    accel_x = []
    accel_y = []
    accel_z = []
    tot_len = 0
    num_row = 0
    for row in data:
        string = row[3]
        l = re.split(';|,|:', string)
        place_holder = 0
        x = []
        y = []
        z = []
        for i,el in enumerate(l):
            if place_holder == 2:
                x.append(float(el))
            if place_holder == 3:
                y.append(float(el))
            if place_holder == 4:
                z.append(float(el))
                place_holder = -1
            place_holder += 1

        if len(x) >= 5:
            accel_x.append(x)
            accel_y.append(y)
            accel_z.append(z)
            tot_len += len(x)
            num_row += 1

    #avg_len = int(tot_len/num_row)
    #had avg_len = 13 for gesture 2 and avg_len=10 for gesture 3 but want val to work the same for both
    avg_len = 12
    x_corr = np.zeros(avg_len)
    y_corr = np.zeros(avg_len)
    z_corr = np.zeros(avg_len)

    #wouldn't need another for loop but need tot_len to get avg_len
    for i in range(num_row):
        x_corr += np.array(scale(accel_x[i], avg_len, linear))/num_row
        y_corr += np.array(scale(accel_y[i], avg_len, linear))/num_row
        z_corr += np.array(scale(accel_z[i], avg_len, linear))/num_row
    xs = x_corr.tolist()
    ys = y_corr.tolist()
    zs = z_corr.tolist()
    # xs = offset_and_normalize(x_corr.tolist())
    # ys = offset_and_normalize(y_corr.tolist())
    # zs = offset_and_normalize(z_corr.tolist())
    return xs,ys,zs

def offset_and_normalize(inp):
    x_hat = 0
    for x in inp:
        x_hat+=x
    x_hat = x_hat/len(inp)
    x_sq = 0
    for x in inp:
        x_sq += (x-x_hat)**2
    denom = x_sq**0.5
    result = []
    for x in inp:
        result.append((x-x_hat)/denom)
    return result


d2 = get_corr(data2)
d3 = get_corr(data3)

x2,y2,z2 = d2
x3,y3,z3 = d3



# x2,y2,z2 = d2
# x3,y3,z3 = d3



