import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import {
  DynamoDBDocumentClient,
  ScanCommand,
  PutCommand,
  GetCommand,
} from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});
const dynamo = DynamoDBDocumentClient.from(client);
const tableName = "IoT_DB";

export const handler = async (event, context) => {
  let body;
  let statusCode = 200;
  const headers = {
    "Content-Type": "application/json",
  };

  try {
    switch (event.routeKey) {
      case "GET /items":
        body = await dynamo.send(
          new ScanCommand({ TableName: tableName })
        );

        if (!body.Items || body.Items.length === 0) {
          return {
            statusCode: 404,
            body: JSON.stringify({ message: "No items found in the table" }),
          };
        }

        const groupedData = body.Items.reduce((acc, item) => {
          const cname = item.cname;
          const ctemp = item.ctemp;
          const chumid = item.chumid;

          if (!acc[cname]) {
            acc[cname] = { ctempSum: 0, chumidSum: 0, count: 0, firstCreated: item.id };
          }

          acc[cname].ctempSum += ctemp;
          acc[cname].chumidSum += chumid;
          acc[cname].count += 1;

          return acc;
        }, {});

        const groupedArray = Object.entries(groupedData).map(([cname, stats]) => ({
          cname,
          avgCtemp: Math.round(stats.ctempSum / stats.count),
          avgChumid: Math.round(stats.chumidSum / stats.count),
          count: stats.count,
          firstCreated: stats.firstCreated, 
        }));

        const sortedData = groupedArray
          .sort((a, b) => {
            if (b.count === a.count) {
              return a.firstCreated < b.firstCreated ? -1 : 1; 
            }
            return b.count - a.count; 
          })
          .slice(0, 4);

        body = sortedData.map((item) => ({
          cname: item.cname,
          avgCtemp: item.avgCtemp, 
          avgChumid: item.avgChumid,
        }));

        break;
      case "PUT /items":
        let requestJSON = JSON.parse(event.body);
        await dynamo.send(
          new PutCommand({
            TableName: tableName,
            Item: {
              id: String(Date.now()),
              cname: requestJSON.state.desired.cname,
              ctemp: requestJSON.state.desired.ctemp,
              chumid: requestJSON.state.desired.chumid,
            },
          })
        );
        body = `Put item ${requestJSON.id}`;
        break;
      default:
        throw new Error(`Unsupported route: "${event.routeKey}"`);
    }
  } catch (err) {
    statusCode = 400;
    body = err.message;
  } finally {
    body = JSON.stringify(body);
  }

  return {
    statusCode,
    body,
    headers,
  };
};
