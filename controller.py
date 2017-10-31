#controller that saves data from the ESP8266 and ATMega328p, gets data from its
#webpage where settings are stored and returns command to the ESP/ATMega328p.
#
#Uses mqtt library, a sqlite database, a web server
#
#sends message to arduino via mqtt with topic as follows:
# espName so that an ESP can easily formward the message after verifying the authenticity of the message

import  os.path, sqlite3, logging
import Crypto.Cipher.AES as aes
import Crypto.Cipher as cyph
from Crypto.Hash import SHA, HMAC
import binascii
import paho.mqtt.client as mqtt
from datetime import datetime as dt
import ipaddress as ip

# DB_FILE = "/home/pittix/hdd/uni2/Domotics/controller/database.sqlite" # database sqlite3 file
DB_FILE = "/home/pittix/Archivi/Domotics/database.sqlite" # database sqlite3 file
IVLEN = 16 #number of hex digits for the initialization vector
class MainController():
    pass


class DatabaseActions():
    #db constants
    DATA_CONFIG=0
    DATA_CYPHER=1
    DATA_CYPHER_CONFIG=2
    DATA_ESP_NAMES=3
    DATA_ROOM = 4
    #store values
    DATA_TEMP = 5
    DATA_STATUS = 6
    DATA_UPDATE_ALL=7

    def __init__(self,connect=False):
        if connect is True:
            database_connect()

    def disconnect(self):
        self.connect.commit()
        self.connect.close()

    def database_connect(self):
        #apparently it creates automatically the db, so I create tables needed
        #table creation -- TIP createIfNotExists
        conn = sqlite3.connect(DB_FILE)
        curs = conn.cursor()
        #rooms name, data must be inserted manually
        curs.execute('''CREATE TABLE IF NOT EXISTS rooms(id integer primary key, name text unique, zone integer)''')
        #arduinos table. data can be initialized manually or derived from esp
        curs.execute('''CREATE TABLE  IF NOT EXISTS
            arduinos(id integer primary key,SN text,roomID integer , status integer,
            espID integer,curConfig integer,controlsZone integer, foreign key (roomID) REFERENCES rooms(id))''')
        #esp list. data should be initialized at the beginning
        curs.execute('''CREATE TABLE  IF NOT EXISTS esp(id integer primary key,
            roomID integer, name text unique, status integer , esp_ip text,
            esp_dataKey text,esp_ivKey text,esp_passphrase text, esp_staticIV text,
            foreign key (roomID) REFERENCES rooms(id))''')
        #stores the temperatures
        curs.execute('''CREATE TABLE  IF NOT EXISTS recordedTemperature(id integer primary key,
                temperature float,arduinoID integer,
                recordTime timestamp, foreign key (arduinoID) REFERENCES arduinos(id) )''')
        #current configuration for each arduino and ESP
        curs.execute('''CREATE TABLE  IF NOT EXISTS setConfig(id integer primary key,
                    minTemp float,maxTemp float, roomID integer, roomEnabled boolean default True ,
                    isEsp boolean,foreign key (room) REFERENCES rooms(id))''')
        #program some temperatures
        curs.execute('''CREATE TABLE  IF NOT EXISTS program(id integer primary key,minTemp float,maxTemp float,
         room integer, startTime timestamp, endTime timestamp,foreign key (room) REFERENCES rooms(id))''')
        conn.commit()
        self.connection = conn

    def get_data_from_db(self,espN,reqType):
        """ return the data from the database regarding an ESP or ARDUINO, depending on the data type
        if the type is:
            DATA_CONFIG, returns the
            DATA_CIPHER, returns the keys for both the IV and the data and the passphrase to generate the hash of the message
            DATA_CIPHER_CONFIG returns together the two before
            DATA_ESP_NAMES return the name of all the ESPs
            DATA_ROOM return the room id to which an arduino is connected
            DATA_ROOM_ALL returns a list of tuples with (arduinoID,roomID)
        """
        cursor = self.connect.cursor()
        if(reqType == self.DATA_CONFIG):
            ret=cursor.execute("SELECT arduinos.SN,arduinos.curConfig FROM esp LEFT JOIN arduinos ON esp.id=arduinos.espID WHERE esp.name=?",espN)
        elif(reqType == self.DATA_CIPHER):
            #data request
            ret = cursor.execute("SELECT esp_dataKey,esp_ivKey,esp_passphrase,esp_staticIV FROM esp WHERE name is ? ",topic)
        elif(reqType == self.DATA_CIPHER_CONFIG):
            ret = cursor.execute("SELECT id,esp_dataKey,esp_ivKey,esp_passphrase,esp_staticIV FROM esp WHERE name is ? ",topic)
            arduinos = cursor.execute("SELECT id FROM arduinos WHERE esp_num IS ? ",ret[0])
        elif(reqType == self.DATA_ESP_NAMES):
            ret= cursor.execute("SELECT name FROM esp")
        elif(reqType == self.DATA_ROOM):
            ret = cursor.execute("SELECT roomID FROM arduinos WHERE espID IS ? ",espN)
        elif(reqType == self.DATA_ROOM_ALL):
            ret = cursor.execute("SELECT id,roomID FROM arduinos")
        elif(reqType==self.DATA_UPDATE_ALL):
            ret=cursor.execute("SELECT arduinos.SN,arduinos.curConfig FROM esp LEFT JOIN arduinos ON esp.id=arduinos.espID WHERE esp.name=?",espN)
        else:
            log.warning("In DatabaseActions.get_data_from_db no valid type was given, %s ",reqType)
        #disconnect from db
        cursor.disconnect()
        return ret
    def store_data_to_db(self,ardN,data, dataType):
        """Expect ardN as the id of the arduino and data as the data from the ESP regarding a specific arduino. If ardN is a list, data must be a dictionary of lists where the key fied of the dictionary is the id of the arduino and the value is the data bytes regarding that arduino.
        if dataType is:
             DATA_TEMP, value must be given in a list of tuples like (temperature,ardID,isESP,time)"""
        conn=self.database_connect().cursor()
        if(dataType == self.DATA_TEMP):
            for el in data:
                conn.execute("INSERT INTO recordedTemperature VALUES (NULL,?,?,?,?)",el[0],el[1],el[2],el[3])
        elif(dataType == self.DATA_STATUS):
            conn.execute("UPDATE TABLE arduinos WHERE ardID IS ?  VALUE status=?",ardID,data) #TODO check!!
        else:
            log.warning("In DatabaseActions.store_data_to_db no valid type was given, %s ",dataType)

class ConnectionHandler():
    #edit for your configuration. socket of the mqtt broker
    broker_IP=ip.ip_address('127.0.0.1')
    broker_port = 1883


    def __init__(self):
        self.db_conn = DatabaseActions()

        self.conn=mqtt.connect(broker_IP,broker_port,keepalive=30,bind_address="127.0.0.1")

        subscription =db_conn.get_data_from_db(None,reqType=DATA_ESP_NAMES)
        for sub in subscription:
            mqtt.subscribe(sub,1) #receive temperature from each arduino

        #on_message of the heating system. Messages from the ESP connected to the boiler. Special parsing

    def sendTo(espName,data):
        """sends encrypted data to a specific esp. there's one byte for each connected arduino preceded by one byte identifying it """
        (dataK,IV_K,passphrase,staticIV)=db_conn.get_data_from_db(espName,DATA_CYPHER)
        #cypher data
        iv =binascii.b2a_hex(os.urandom(IV_LEN/2)) #need to generate an IV for the cypher
        #enc IV
        encIV = aes.new(IV_K,mode=aes.MODE_CBC,IV=staticIV) # create the cypher
        iv_enc = encIV.encrypt(iv)
        #decr cliID with iv
        encCliID =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        cliID_enc = encCliID.encrypt(Byte('r'))
        #decr data
        encrData =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        data_enc = encrData.encrypt(dataConf)
        message=iv_enc+","+cliID_enc+","+data_enc # concatenate data with comma as a separator
        hmac = HMAC.new(passphrase,message,SHA)
        computed_hash = hmac.hexdigest()
        data=message + ","+computed_hash
        #send data to the esp
        mqtt_broker.publish(espName,data)

    def handleData(client,userdata,message):
        """Receives data from one ESP and then decides what to do with it (if it's temperature info, stores it in the database for example)"""
        topic = message.topic
        payl = message.payload
        if(topic[-2:]("SN")):
            update_arduinos(topic,payl)

        (dataK,IV_K,passphrase,staticIV)=get_data_from_db(topic,DATA_CYPHER)
        #uncypher data, check Auth+Integrity
        data = payl.split(",")
        # extract list items
        encrypted_iv = binascii.unhexlify(data[0])
        encrypted_nodeid = binascii.unhexlify(data[1])
        encrypted_data = binascii.unhexlify(data[2])
        received_hash = binascii.unhexlify(data[3])
        #decr IV
        decIV = aes.new(IV_K,mode=aes.MODE_CBC,IV=staticIV) # create the cypher
        iv = decIV.decrypt(encrypted_iv)
        #decr cliID with iv
        decrCliID =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        cliID = decrCliID.decrypt(encrypted_nodeid)
        #decr data
        decrData =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        data = decrData.decrypt(encrypted_data)
        #verify correctness with hash function
        fullmessage = s.join([cliID,iv,data,sessionID])
        hmac = HMAC.new(passphrase,fullmessage,SHA)
        computed_hash = hmac.hexdigest()
        if(not(received_hash == computed_hash)):
            log.error("message arrived from %s has not the same hash as the one computed")
            return
        #if no error happened
        #extract temperature and status from data
        #data is made out of blocks with 7 bytes each: 1st byte is the arduinoID, byte[2-6] are the temp and the 7th byte is the arduino status
        for i in range(len(data)/7):
            ardID=data[0+1]
            temp = data[1+i:5+i]
            status = data[6+i]
            temperatures=[]
            now = dt.now()
            if ardID == 0:
                isEsp = True
            else:
                isEsp=False
            recordTimes=[]
            deltaAcq = dt.timedelta(minutes=1)#prepare to store the data acquired with 1 minutes delay
            for j in range(len(temp)):
                temperatures.append( int(temp[j])/10.0 )
                recordTimes.append((now-j*deltaAcq)) # time when the temperature was acquired
            #store data into db
            roomID=db_conn.execute("SELECT room FROM arduinos WHERE id is ?",ardID)
            db_conn.executemany("INSERT INTO recordedTemperature VALUES (NULL,?,?,?,?)",temperatures,roomID,isEsp,recordTimes)
            db_conn.commit() #executes the storing of recorded temperatures



    def update_key_ivs(self,espN):
        """Periodically update the keys and the  staticIVs of each ESP to avoid collisions"""
        #TODO
        return
    #maybe useless as I need to know where the arduino was placed
    # def update_arduinos(self,topic,message):
    #     """Add arduinos connected to an ESP, so that the arduino """
    #     return
class DecisionMaker():
    db_conn = DatabaseActions()
    def update_arduino_config(self,espN):
        """From the new data in the database, update the arduino config and store it in the database"""
        db_conn.connect()
        arduinos_rooms = db_conn.get(DatabaseActions.DATA_UPDATE_ALL)
        #last room temperatures - relay version
        curs=db_conn.connect()
        now= dt.now()
        prev = now - dt.timedelta(minutes=5)# 5 minutes interval

        # extract all the programming running right now
        program = curs.execute('''SELECT roomID,minTemp,maxTemp FROM setConfig
            WHERE startTime < time('now') and endTime > time('now')''')# returns all configurations for each room or zone
        #returns a tuple with the arduino id, the room id, the [minimum, average, maximum] temperature of that room in the last 5 minutes
        roomTemps=curs.execute('''SELECT a.id, a.room,
        	min(t.temperature),	avg(t.temperature),	max(t.temperature),
            sc.minTemp,sc.maxTemp,
            FROM  rooms as r
            LEFT JOIN arduinos as a on r.id = a.room
            LEFT JOIN recordedTemperature as t  on a.id= t.arduinoID
            LEFT JOIN program as p on r.id = p.room
            LEFT JOIN setConfig as sc on r.id = sc.room
            where sc.roomEnabled=1
            and t.recordTime BETWEEN ? and ?
            and p.startTime< time('now')  and p.endTime> time('now')
            group by  p.room''',prev,now)
        
         x for pr in program if pr[0]

    def send_boiler_setting(self,zone,HeatTemp,waterTemp):
        pass

    def send_arduino_config(self,espN):
        """When an esp is waken and sended data, transmits the updated config to it for all the arduinos connected to it"""
        dataConf = db_conn.get_data_from_db(espName,DATA_CONFIG)
        message=""
        for SN,conf in dataConf:
            message=SN+":"+conf+"\n"
        ConnectionHandler.sendTo(espN,message)

def __main__():
    ConnectionHandler.mqtt_start()
    DatabaseActions.database_connect()

if __name__ == "__main__":
    db=DatabaseActions()
    db.database_connect()
