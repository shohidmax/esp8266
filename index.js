const express = require('express');
const cors = require('cors');
const jwt = require('jsonwebtoken');
const multer = require('multer');
const bodyParser = require('body-parser');
// const { createCanvas, loadImage } = require('canvas');
require('dotenv').config();

const { MongoClient, ServerApiVersion, ObjectId, ISODate } = require('mongodb');
const { error } = require('console');
const app = express();
const port = process.env.PORT || 3005;


const http = require('http').createServer(app);
const io = require('socket.io')(http);
app.use(express.static('public'));
//----------------------avatar api resorce -----------

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// Set up multer for handling file uploads
const storage = multer.memoryStorage();
const upload = multer({ storage: storage });
// -----------------avatar api close ----------

app.use(cors());
app.use(express.json());

const uri = "mongodb+srv://atifsupermart202199:FGzi4j6kRnYTIyP9@cluster0.bfulggv.mongodb.net/?retryWrites=true&w=majority";
const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true, serverApi: ServerApiVersion.v1 });

async function node() {
 try {
  
  
 } catch (error) {
  
 }
 
  
}
node();
async function run() {
  try {
    await client.connect();
    console.log('db connected');
    const productCollection = client.db('atifdatamax').collection('product');
    const productsCollection = client.db('atifdatamax').collection('products');
    const products_2Collection = client.db('atifdatamax').collection('product_2');
    const ESP_2Collection = client.db('atifdatamax').collection('Tempareturedata');
    const bat_2Collection = client.db('atifdatamax').collection('batcuring');
    const TrackDATACollection = client.db('atifdatamax').collection('TrackDATA');



    app.post('/api/data2', async (req, res) => {
      console.log("Received data from NodeMCU:");
      console.log(req.body); // NodeMCU থেকে পাঠানো JSON ডেটা এখানে পাওয়া যাবে
    
      // এখানে ডেটা ডাটাবেসে সংরক্ষণ করতে পারেন বা অন্য কোনো কাজ করতে পারেন
      const { sensor1, sensor2, timestamp } = req.body;
      const accounts = req.body;
      if (sensor1 && sensor2 && timestamp) {
         console.log(`Timestamp: ${timestamp}`);
         console.log(`Sensor 1 (DHT11) - Temp: ${sensor1.temperature}°C, Hum: ${sensor1.humidity}%`);
         console.log(`Sensor 2 (DHT22) - Temp: ${sensor2.temperature}°C, Hum: ${sensor2.humidity}%`);
         // সফলভাবে ডেটা গ্রহণের পর একটি রেসপন্স পাঠান
         res.status(200).json({ message: "Data received successfully" });
         const result = await  ESP_2Collection.insertOne(accounts);
      res.send(result)
      } else {
        res.status(400).json({ message: "Invalid data format" });
      }
    });
    app.post('/api/data1', async (req, res) => {
      console.log("Received data from NodeMCU:");
      console.log(req.body); // NodeMCU থেকে পাঠানো JSON ডেটা এখানে পাওয়া যাবে
    
      // এখানে ডেটা ডাটাবেসে সংরক্ষণ করতে পারেন বা অন্য কোনো কাজ করতে পারেন
      const { sensor1, sensor2, timestamp } = req.body;
      const accounts = req.body;
      if (sensor1 && sensor2 && timestamp) {
         console.log(`Timestamp: ${timestamp}`);
         console.log(`Sensor 1 (DHT11) - Temp: ${sensor1.temperature}°C, Hum: ${sensor1.humidity}%`);
         console.log(`Sensor 2 (DHT22) - Temp: ${sensor2.temperature}°C, Hum: ${sensor2.humidity}%`);
         // সফলভাবে ডেটা গ্রহণের পর একটি রেসপন্স পাঠান
         res.status(200).json({ message: "Data received successfully" });
         const result = await bat_2Collection.insertOne(accounts);
      res.send(result)
      } else {
        res.status(400).json({ message: "Invalid data format" });
      }
    });


    app.get('/api/esptemp', async(req, res) =>{
      const query = {};
      const cursor = ESP_2Collection.find(query);
      const accounts = await cursor.toArray();
      res.send(accounts);
    })
    app.get('/api/trackdata', async(req, res) =>{
      const query = {};
      const cursor = TrackDATACollection.find(query);
      const accounts = await cursor.toArray();
      res.send(accounts);
    })
    app.get('/api/esptemp1', async(req, res) =>{
      const query = {};
      const cursor =  bat_2Collection.find(query);
      const accounts = await cursor.toArray();
      res.send(accounts);
    })
    app.post('/api/esptempu', async (req, res) => {
      const accounts = req.body;
      const result = await  ESP_2Collection.insertOne(accounts);
      res.send(result)
    }); 
    app.post('/api/esptempu1', async (req, res) => {
      const accounts = req.body;
      const result = await bat_2Collection.insertOne(accounts);
      res.send(result)
    }); 
    app.post('/api/trackdata', async (req, res) => {
      const accounts = req.body;
      const result = await TrackDATACollection.insertOne(accounts);
      res.send(result)
    }); 

    app.get('/api/accounts/:id',   async(req, res) =>{
      const id = req.params.id;
      const query = {_id: ObjectId(id)};
      const booking = await accountsCollection.findOne(query);
      res.send(booking);
     })
    //     product display
    // ----------------------------------------------------------------
 
 

    // ----------------- avtar api making close -------------------------



    app.get('/api/accounts/:id',   async(req, res) =>{
     const id = req.params.id;
     const query = {_id: ObjectId(id)};
     const booking = await accountsCollection.findOne(query);
     res.send(booking);
    })
    
    app.delete('/api/data/:id', async (req, res) => {
      const id = req.params.id;
      const query = { _id: ObjectId(id) };
      const result = await noteCollection.deleteOne(query);
      res.send(result); 

    })
   
    
    // ----------- Final  sales Data ------
    app.put('/finalsale', async (req, res) => {
      // const id = req.params.id; 
      const updatedStock = req.body;
      const arry = updatedStock.Sale_Data; 
      const query = {};
      const cursor = SaleCollection.find(query);
      const sale = await cursor.toArray();  
      const sdate = await new Date(updatedStock.Sale_Date);
      const edate = await new Date(updatedStock.Sale_Date);
      const filterdate = await sale.filter(a => {
        const date = new Date(a.date);
        return (date >= sdate && date <= edate);
      });
      console.log(filterdate, '/////////////////-----------------------');
 
      let invoice_list = [];
      console.log(invoice_list, sale.length,  'sale data ---------');
      //------------------------------------
      if (sale.length == 0 ) {  
        console.log('-----------------------------log if  ----------------');

        for await (const pro of arry) {
          const ID = pro._id;
          // console.log(pro._id, pro.StockQty - pro.orderq, pro);
          const update = pro.StockQty - pro.orderq;
          const filter = { _id: ObjectId(ID) };
          const options = { upsert: true };
          const updatedDoc = {
            $set: {
              StockQty: update
            }
          };
          const result = await productsCollection.updateOne(filter, updatedDoc, options);
          console.log(result);
        }
        // ---------------------------------------------- for  update  --------------
        const result = await SaleCollection.insertOne(updatedStock);
   
      } 
      else{
        console.log('-----------------------------else   ----------------');
        
        for await (const inv of sale){
          invoice_list=[...invoice_list, inv.Sale_Invoice.slice(-5)]
           console.log(inv.Sale_Invoice.slice(-5));
        }
        const new_inv_num = (Math.max(...invoice_list) + 1).toString();
        const chng = updatedStock.Sale_Invoice.slice(-5)
        updatedStock.Sale_Invoice = updatedStock.Sale_Invoice.replace(chng, new_inv_num)

        for await (const pro of arry) {
          const ID = pro._id;
          // console.log(pro._id, pro.StockQty - pro.orderq, pro);
          const update = pro.StockQty - pro.orderq;
          const filter = { _id: ObjectId(ID) };
          const options = { upsert: true };
          const updatedDoc = {
            $set: {
              StockQty: update
            }
          };
          const result = await productsCollection.updateOne(filter, updatedDoc, options);
          console.log(result);
        }
        const results = await SaleCollection.insertOne(updatedStock);

        // ---------------------------------------------- for  update  --------------
        // const result = await SaleCollection.insertOne(updatedStock);
  
      }
      
        
      //------------------------------------

      
      // ---------------------------- update close ---------------------------------- 
      res.send({ 'data': 'succesfully data updated',updatedStock  });
    })
 
    
  }
  finally {

  }

}
run().catch(console.dir);




app.get("/", (req, res) => {
  res.send(`<h1 style="text-align: center;
      color: red;"> Server is Running at <span style="color: Blue;">${port}</span></h1>`);
});

app.listen(port, () => {
  console.log("Atif super  mart server running at  : ", port);
});


 

//  function previous_year_artist(req, res, next) {
//         var dateTimeTofilter = moment().subtract(1, 'year');
//         var filter = {
//             "date_added": {
//                 $gte: new Date(dateTimeTofilter._d)
//             }
//         };
//         db.collection.find(
//             filter
//         ).toArray(function(err, result) {
//             if (err) return next(err);
//             res.send(result);
//         });

//     }
