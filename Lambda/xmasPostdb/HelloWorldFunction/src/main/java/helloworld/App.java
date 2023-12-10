package helloworld;

import com.amazonaws.services.dynamodbv2.AmazonDynamoDB;
import com.amazonaws.services.dynamodbv2.AmazonDynamoDBClientBuilder;
import com.amazonaws.services.dynamodbv2.document.DynamoDB;
import com.amazonaws.services.dynamodbv2.document.Item;
import com.amazonaws.services.dynamodbv2.document.PutItemOutcome;
import com.amazonaws.services.dynamodbv2.document.spec.PutItemSpec;
import com.amazonaws.services.dynamodbv2.model.ConditionalCheckFailedException;
import com.amazonaws.services.lambda.runtime.Context;
import com.amazonaws.services.lambda.runtime.RequestHandler;

public class App implements RequestHandler<Thing, String> {

    private DynamoDB dynamoDb;
    private String DYNAMODB_TABLE_NAME = "temp_tree";

    @Override
    public String handleRequest(Thing input, Context context) {
        this.initDynamoDbClient();

        persistData(input);
        return "Success in storing to DB!";
    }

    private PutItemOutcome persistData(Thing thing) throws ConditionalCheckFailedException {


        return this.dynamoDb.getTable(DYNAMODB_TABLE_NAME)
                .putItem(new PutItemSpec().withItem(new Item().withPrimaryKey("buttonID", thing.buttonID)
                        .withString("temperature", thing.state.reported.temperature)
                        .withString("LED", thing.state.reported.LED)
                        .withInt("max", thing.state.reported.max)
                        .withInt("min",thing.state.reported.min)));
    }

    private void initDynamoDbClient() {
        AmazonDynamoDB client = AmazonDynamoDBClientBuilder.standard().
                withRegion("us-east-1").build();

        this.dynamoDb = new DynamoDB(client);
    }

}

class Thing {

    public State state = new State();
    public Integer buttonID;

    public class State {
        public Tag reported = new Tag();
        public Tag desired = new Tag();

        public class Tag {

            public String LED;
            public int min;
            public int max;
            public String temperature;
        }
    }

}