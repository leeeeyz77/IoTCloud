package helloworld;
import java.text.SimpleDateFormat;
import java.util.TimeZone;

import com.amazonaws.services.dynamodbv2.AmazonDynamoDB;
import com.amazonaws.services.dynamodbv2.AmazonDynamoDBClientBuilder;
import com.amazonaws.services.dynamodbv2.document.DynamoDB;
import com.amazonaws.services.dynamodbv2.document.Item;
import com.amazonaws.services.dynamodbv2.document.spec.PutItemSpec;
import com.amazonaws.services.dynamodbv2.model.ConditionalCheckFailedException;
import com.amazonaws.services.lambda.runtime.Context;
import com.amazonaws.services.lambda.runtime.RequestHandler;

public class App implements RequestHandler<Document, String>{//Lambda 함수의 입력은 Document

    private DynamoDB dynamoDb;//DynamoDB 객체
    private String DYNAMODB_TABLE_NAME  = "Logging";// 테이블 이름

    //handleRequest
    @Override
    public String handleRequest(Document input, Context context){
        this.initDynamoDbClient();
        context.getLogger().log("Input: " + input);

        //return null
        return persistData(input);
    }

    private String persistData(Document document) throws ConditionalCheckFailedException{

        //timestamp가 실제 몇 년 몇월 며칠인지를 변환시키는 코드
        //이런 형식으로 바꾸겠다하는 Format 객체
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        //Format 객체 TimeZone
        sdf.setTimeZone(TimeZone.getTimeZone("Asia/Seoul"));
        
        //ms로 변환된 값
        String timeString = sdf.format(new java.util.Date(document.timestamp*1000));

        //온도 정보, 열선패드 상태, 3색 LED 상태가 이전 상태와 동일한 경우 테이블에 저장하지 않고 종료
        if(document.current.state.reported.temperature.equals(//온도 정보
                document.previous.state.reported.temperature)&&
                document.current.state.reported.LED3.equals(//3색 LED
                document.previous.state.reported.LED3)&&
                document.current.state.reported.WarmPad.equals(//열선패드
                document.previous.state.reported.WarmPad)&&
                document.current.state.reported.WarmPadState.equals(
                document.previous.state.reported.WarmPadState
                )){
                    return null;
        }

        //테이블 가져오기(데이터에 항목을 put)
        return this.dynamoDb.getTable(DYNAMODB_TABLE_NAME)
                .putItem(new PutItemSpec().withItem(new Item()
                        .withPrimaryKey("deviceId", document.device) //파티션키
                        .withLong("time", document.timestamp) //정렬키
                        .withString("temperature", document.current.state.reported.temperature) //온도정보
                        .withString("LED3", document.current.state.reported.LED3) //3색 LED
		                .withString("WarmPad", document.current.state.reported.WarmPad) //열선패드 온도 설정
                        .withString("WarmPadState", document.current.state.reported.WarmPadState)//열선패드 온오프 상태
                        .withString("timestamp", timeString))) //timestamp
		        .toString();
    }

        private void initDynamoDbClient(){
            AmazonDynamoDB client = AmazonDynamoDBClientBuilder.standard()
                    .withRegion("ap-southeast-2").build(); //지역변경 - 시드니

            this.dynamoDb = new DynamoDB(client);
        }

}

class Document{
    //JSON 형식의 상태 문서는 2개의 기본 노드를 포함합니다: previous 및 current
    public Thing previous;//previous 노드에는 업데이트가 수행되기 전의 전체 섀도우 문서의 내용이 포함
    public Thing current;//current에는 업데이트가 성공적으로 적용된 후의 전체 섀도우 문서 포함
    public long timestamp;//상태 문서가 생성된 시간 정보
    public String device; //AWS IoT 등록된 사물 이름(상태문서에 포함된 값은 아니고, iot 규칙을 통해서 Lambda 함수로 전달된 값) - 이 값을 해당 규칙과 관련된 사물 이름을 나타냄
}

class Thing{
    public State state = new State(); //state 필드
    public long timestamp; //timestamp 필드
    public String clientToken; //reported 속성이나,

    public class State{//desired 속성도 포함될 수 있음
        public Tag reported = new Tag();
        public Tag desired = new Tag();

        public class Tag{//reported, desired의 각각은 Tag 값으로 temperature, LED3, WarmPad, WarmPadState를 속성으로 가짐
            public String temperature;
            public String LED3;
            public String WarmPad;
            public String WarmPadState;
        }
    }
}

