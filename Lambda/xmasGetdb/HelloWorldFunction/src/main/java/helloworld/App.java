package helloworld;

import com.amazonaws.services.lambda.runtime.Context;
import com.amazonaws.services.lambda.runtime.RequestHandler;
import com.amazonaws.services.dynamodbv2.AmazonDynamoDB;
import com.amazonaws.services.dynamodbv2.AmazonDynamoDBClientBuilder;
import com.amazonaws.services.dynamodbv2.document.DynamoDB;
import com.amazonaws.services.dynamodbv2.document.Item;
import com.amazonaws.services.dynamodbv2.document.Table;

import java.util.HashMap;
import java.util.Map;

public class App implements RequestHandler<Map<String, Object>, Map<String, Object>> {

    private static final String TABLE_NAME = "temp_tree";
    private final AmazonDynamoDB dynamoDBClient;
    private final DynamoDB dynamoDB;

    public App() {
        dynamoDBClient = AmazonDynamoDBClientBuilder.standard().build();
        dynamoDB = new DynamoDB(dynamoDBClient);
    }

    @Override
    public Map<String, Object> handleRequest(Map<String, Object> input, Context context) {
        int buttonId = Integer.parseInt(input.getOrDefault("buttonID", "1").toString());

        // Retrieve data from DynamoDB
        Map<String, Object> temperatureRange = getDataFromDynamoDB(buttonId);

        return temperatureRange;
    }

    private Map<String, Object> getDataFromDynamoDB(int buttonId) {
        Table table = dynamoDB.getTable(TABLE_NAME);

        Item item = table.getItem("buttonID", buttonId);
        if (item != null) {
            // Item found, return the data
            Map<String, Object> temperatureRange = new HashMap<>();
            temperatureRange.put("min", item.getInt("min"));
            temperatureRange.put("max", item.getInt("max"));
            return temperatureRange;
        } else {
            // Item not found
            return null;
        }
    }
}
