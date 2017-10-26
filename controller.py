#controller that saves data from the ESP8266 and ATMega328p, gets data from its
#webpage where settings are stored and returns command to the ESP/ATMega328p.
#
#Uses mqtt library, a sqlite database, a web server
#
#sends message to arduino via mqtt with topic as follows:
# espName so that an ESP can easily formward the message after verifying the authenticity of the message

import  os.path, sqlite3, logging
import Crypto.Cipher.AES as aes
import Crypto.Ciph as cyph
from Crypto.Hash import SHA, HMAC
import binascii
import paho.mqtt.client as mqtt
import datetime.datetime as dt

DB_FILE = "/home/pittix/hdd/uni2/Domotics/controller/database.sqlite" # database sqlite3 file

class MainController():



class DatabaseActions():
    #db constants
    self.DATA_CONFIG=0
    self.DATA_CYPHER=1
    self.DATA_CYPHER_CONFIG=2
    self.DATA_ESP_NAMES=3
    self.DATA_ROOM = 4
    #store values
    self.DATA_TEMP = 5
    self.DATA_STATUS = 6


    def database_connect(self):
        #apparently it creates automatically the db, so I create tables needed
        if(os.isfile(DB_FILE)):
            try:
                conn = sqlite3.connect(db_file)
                print(sqlite3.version)
            except Error as e:
                print(e)
            curs = conn.cursor()
            return curs
        else:
            #table creation -- TIP createIfNotExists
            curs.execute("CREATE TABLE rooms(id integer primary key,name varchar unique, device integer, isEsp boolean )")
            curs.execute("CREATE TABLE arduinos(id integer primary key,room integer foreign key rooms.id , status integer , espID integer)")
            curs.execute("CREATE TABLE esp(id integer primary key, room integer foreign key rooms.id, name varchar unique, status integer , esp_ip varchar, esp_dataKey varchar,esp_ivKey varchar,esp_passphrase varchar, esp_staticIV varchar)")
            curs.execute("CREATE TABLE recordedTemperature(id integer primary key,temperature float,station integer, isEsp boolean ,recordTime datetime)")
            curs.execute("CREATE TABLE setTemperature(id integer primary key,temperature float,room integer, isEsp blob)")
            #ROOM TABLE
            rooms = [(NULL, "parents",NULL,NULL),(NULL, "child",NULL,NULL), (NULL, "bathroomBig",NULL,NULL),(NULL, "bathroomSmall",NULL,NULL),(NULL, "homeoffice",NULL,NULL),(NULL, "entrance",NULL,NULL),(NULL, "livingroom",NULL,NULL)]
            rooms.append([ (NULL, "kitchen",NULL,NULL), (NULL, "preBedroom",NULL,NULL) ])
            #fill tables
            curs.executemany("INSERT INTO rooms VALUES(NULL,?,?,)")
            #update status from arduino name
            curs.execute("UPDATE arduinos SET status=? WHERE id=?",status,ardID)
            curs.execute("DELETE FROM arduinos WHERE id=?",status,ardID)
            #ESP TABLE
            esps=[(1,NULL,NULL,"192.168.1.11"),(2,NULL,NULL,"192.168.1.12"),(1,NULL,NULL,"192.168.1.13")]
            return curs

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
        cursor = self.database_connect()
        if(reqType == self.DATA_CONFIG):
            ret = cursor.execute("SELECT id FROM esp WHERE name is ? ",topic)
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
        else:
            log.warning("In DatabaseActions.get_data_from_db no valid type was given, %s ",reqType)
        cursor.execute("SELECT esp.id,arduinos.id FROM esp LEFT JOIN arduinos ON esp.id=arduinos.espID WHERE esp.name=?",espN)
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

class ConnectionHandler(self):
    self.conn=None
    #edit for your configuration. socket of the mqtt broker
    self.broker_IP=ip.ip_address('127.0.0.1')
    self.broker_port = 1883
    self.db_conn = DatabaseActions()


    def __init__(self):
        conn=mqtt.connect(broker_IP,broker_port,keepalive=30,bind_address=127.0.0.1)

        subscription =db_conn.get_data_from_db(None,reqType=DATA_ESP_NAMES)
        for sub in subscription:
            mqtt.subscribe(sub,1) #receive temperature from each arduino

        #on_message of the heating system. Messages from the ESP connected to the boiler. Special parsing

    def sendTo(espName):
        """sends encrypted data to a specific esp. there's one byte for each connected arduino preceded by one byte identifying it """
        (dataK,IV_K,passphrase,staticIV)=db_conn.get_data_from_db(espName,DATA_CYPHER)

        dataConf = db_conn.get_data_from_db(espName,DATA_CONFIG)
        #cypher data
        iv =0 #need to generate an IV for the cypher
        #enc IV
        encIV = aes.new(IV_K,mode=aes.MODE_CBC,IV=staticIV) # create the cypher
        iv_enc = encIV.encrypt(iv)
        #decr cliID with iv
        encCliID =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        cliID_enc = encCliID.encrypt(Byte('r'))
        #decr data
        encrData =aes.new(dataK,mode=aes.MODE_CBC,IV=iv) # create the cypher
        data_enc = encrData.encrypt(dataConf)
        message=s.join([iv_enc,cliID_enc,data_enc])
        hmac = HMAC.new(passphrase,message,SHA)
        computed_hash = hmac.hexdigest()
        data=s.join([iv_enc,cliID_enc,data_enc,computed_hash])
        #send data to the esp
        mqtt_broker.publish(espName,data)

    def handleData(client,userdata,message):
        """Receives data from one ESP and then decides what to do with it (if it's temperature info, stores it in the database for example)"""
        topic = message.topic
        payl = message.payload

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
        """Periodically update the keys and the IVs of each ESP to avoid collisions"""
        return
class DecisionMaker(self):
    def update_arduino_config(self):
        """From the new data in the database, update the arduino config and store it in the database"""

def __main__():
    ConnectionHandler.mqtt_start()
    DatabaseActions.database_connect()
