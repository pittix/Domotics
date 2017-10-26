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
            #table creation
            curs.execute("CREATE TABLE rooms(id integer primary key,name varchar unique, device integer, isEsp blob)")
            curs.execute("CREATE TABLE arduinos(id integer primary key,room integer unique, status blob , esp_num integer)")
            curs.execute("CREATE TABLE esp(id integer primary key,room varchar unique, name varchar unique, status blob , esp_ip text, esp_dataKey varchar,esp_ivKey varchar,esp_passphrase varchar, esp_staticIV varchar)")
            curs.execute("CREATE TABLE recordedTemperature(id integer primary key,temperature float,station integer, isEsp blob,recordTime varchar)")
            curs.execute("CREATE TABLE setTemperature(id integer primary key,temperature float,room integer, isEsp blob)")
            #ROOM TABLE
            rooms = [(NULL, "parents",NULL,NULL),(NULL, "child",NULL,NULL), (NULL, "bathroomBig",NULL,NULL),(NULL, "bathroomSmall",NULL,NULL),(NULL, "homeoffice",NULL,NULL),(NULL, "entrance",NULL,NULL),(NULL, "livingroom",NULL,NULL)]
            rooms.append([ (NULL, "kitchen",NULL,NULL), (NULL, "preBedroom",NULL,NULL) ])

            #ESP TABLE
            esps=[(1,NULL,NULL,"192.168.1.11"),(2,NULL,NULL,"192.168.1.12"),(1,NULL,NULL,"192.168.1.13")]
            return curs

    def get_data_from_db(self,espN,reqType):
        cursor = database_connect()
        if(reqType == DATA_CONFIG):
            ret = cursor.execute("SELECT id FROM esp WHERE name is ? ",topic)
        elif(reqType == DATA_CIPHER):
            #data request
            ret = cursor.execute("SELECT esp_dataKey,esp_ivKey,esp_passphrase,esp_staticIV FROM esp WHERE name is ? ",topic)
        elif(reqType == DATA_CIPHER_CONFIG):
            ret = cursor.execute("SELECT id,esp_dataKey,esp_ivKey,esp_passphrase,esp_staticIV FROM esp WHERE name is ? ",topic)
            arduinos = cursor.execute("SELECT id FROM arduinos WHERE esp_num IS ? ",ret[0])
        elif(reqType == DATA_ESP_NAMES):
            ret cursor.execute("SELECT name FROM esp")

        #disconnect from db
        cursor.disconnect()
        return ret
    def store_data_to_db(self,ardN,data):
        """Expect ardN as the id of the arduino and data as the data from the ESP regarding a specific arduino. If ardN is a list, data must be a dictionary of lists where the key fied of the dictionary is the id of the arduino and the value is the data bytes regarding that arduino"""
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
