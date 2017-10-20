//When page loads, initialize the map
$(document).ready(function () {    
    initMap();
});

//Time interval
$('#fromDate').datetimepicker({
   format: 'y-m-d H:i:s'
});
$('#toDate').datetimepicker({
   format: 'y-m-d H:i:s'
});


var markers = [];
var mapMarkers = [];
var mapCircle = [];
var request, requestRect, map, heatmap;
var northEastLat, northEastLon, southWestLat, southWestLon;                  
var rectangle = null;
var dataReply;

function initMap() {
    //Map borders
   var cent = {lat: 59.860807242880206, lng: 17.63833522796631};

    map = new google.maps.Map(document.getElementById('map'), {
        zoom: 15,
        center: cent
    });

    var borderCorners = [
         {lat: 59.86496784, lng: 17.619806},
         {lat: 59.86496784, lng: 17.6555931124},
         {lat: 59.85688300, lng: 17.6555931124},
         {lat: 59.85688300, lng: 17.619806},
         {lat: 59.86496784, lng: 17.619806}
     ];
 
     var border = new google.maps.Polyline({
         path: borderCorners,
         geodesic: true,
         strokeColor: '#FF0000',
         strokeOpacity: 1.0,
         strokeWeight: 2
      });
     border.setMap(map);

    //If point region, put circles on click
    if(document.getElementById('regType').value == "pointRegion"){
        google.maps.event.addListener(map, 'click', function(e) {
            var marker = new google.maps.Marker({
                position: e.latLng,
                map: map,
                icon: {
                  path: google.maps.SymbolPath.CIRCLE,
                  fillColor: 'green',
                  fillOpacity: .5,
                  strokeColor: 'white',
                  strokeWeight: .5,
                  scale: 10
                }
            });

            mapMarkers.push(marker);      
            markers.push([marker.getPosition().lat(), marker.getPosition().lng()]);

            //Remove the circle on double click
            google.maps.event.addListener(marker, "dblclick", function (e) { 
                this.setMap(null);
                var index = markers.indexOf([marker.getPosition().lat(), marker.getPosition().lng()]);
                markers.splice(index, 1);
            });
        });   
    }

    //If rectangular region, draw rectangular on map
    else if(document.getElementById('regType').value == "rectRegion"){
        var drawingManager = new google.maps.drawing.DrawingManager({
          drawingControl: true,
          drawingControlOptions: {
            position: google.maps.ControlPosition.TOP_CENTER,
            drawingModes: ['rectangle']
          },
          rectangleOptions: {
            strokeWeight: 1,
            clickable: true,
            editable: false,
            zIndex: 1
          }
        });
        drawingManager.setMap(map);

        google.maps.event.addListener(drawingManager, 'overlaycomplete', function(event) {
            if (event.type == google.maps.drawing.OverlayType.RECTANGLE) {
                if(rectangle != null)
                    rectangle.setMap(null);
                rectangle = event.overlay;

                var bounds = rectangle.getBounds();
                northEastLat = bounds.getNorthEast().lat();
                northEastLon = bounds.getNorthEast().lng();
                southWestLat = bounds.getSouthWest().lat();
                southWestLon = bounds.getSouthWest().lng();

                console.log(southWestLon);
                console.log(southWestLat);

                drawingManager.setDrawingMode(null);
            }
        });

        //If drawing mode is changed, clean the map (remove rectangle)
        google.maps.event.addListener(drawingManager, "drawingmode_changed", function() {
            if ((drawingManager.getDrawingMode() == google.maps.drawing.OverlayType.RECTANGLE) && (rectangle != null))
                rectangle.setMap(null);
        });
    }
}

//Form the request for point region
function formRequest(pointArr) {
    var startDate = document.getElementById('fromDate').value;
    var endDate = document.getElementById('toDate').value;
    var dataType = document.getElementById('dataType').value;
    var operation = document.getElementById('operation').value;
    var interval = document.getElementById('interval').value;

    request = 
    {
        "TimeInterval": {
            "From": startDate,
            "To": endDate
        },
        "Region": {
            "Type": "PointRegion",
            "Points": [
            ]
        },
        "Dataset": dataType,
        "Statistics": {
            "Operation": "cNone",
            "Interval": interval,
            "GetAccuracies": false
        }
    }

     for (var i = 0; i < pointArr.length; i++) {
        request['Region']['Points'].push({"Longitude": parseFloat(pointArr[i][1]),"Latitude": parseFloat(pointArr[i][0])});
     }

    console.log(request);
    getPointReg(request);
}

//Form the request for rectangular region
function formRequestRect() {
    var startDate = document.getElementById('fromDate').value;
    var endDate = document.getElementById('toDate').value;
    var dataType = document.getElementById('dataType').value;
    //var operation = document.getElementById('operation').value;
    var interval = document.getElementById('interval').value;

    requestRect = 
    {
        "TimeInterval": {
            "From": startDate,
            "To": endDate
        },
        "Region": {
            "Type": "RectRegion",
            "NorthEast": {
            	"Longitude": northEastLon,
            	"Latitude": northEastLat
            },
            "SouthWest": {
            	"Longitude": southWestLon,
            	"Latitude": southWestLat
            },
            "Resolution": 100
        },
        "Dataset": dataType,
        "Statistics": {
            "Operation": "cNone",
            "Interval": interval,
            "GetAccuracies": false
        }
    }

    console.log(requestRect);
    getRectReg(requestRect);
}



//Remove markers and rectangles from the map
function deleteMarkers() {
    for (i in mapMarkers) {
      mapMarkers[i].setMap(null);
    }
    mapMarkers.length = 0; 
    markers = [];

    for (j in mapCircle) {
      mapCircle[j].setMap(null);
    }
    mapCircle.length = 0; 

    if (rectangle != null)
        rectangle.setMap(null);
}

//AJAX request for point region
function getPointReg() {
    $.ajax({
        url: 'http://data.greeniot.it.uu.se/4dialog/',
		data: JSON.stringify(request),
        dataType: 'json',
        type: 'POST',
        success: function (data, status, xhr) {
            alert('Success!');
            dataReply = data['Replies'];
            console.log(dataReply);
            drawVisualization(dataReply);
        },
        error: function (xhr, status, error) {
            alert('Update Error occurred - ' + error);
        }
    });    
}

//AJAX request for rectangular region
function getRectReg() {
    $.ajax({
        url: 'http://data.greeniot.it.uu.se/4dialog/',
		data: JSON.stringify(requestRect),
        dataType: 'json',
        type: 'POST',
        success: function (data, status, xhr) {
            alert('Success!');
            dataReply = data['Replies'];
            console.log(dataReply);
            drawRectVisualization(dataReply);
        },
        error: function (xhr, status, error) {
            alert('Update Error occurred - ' + error);
        }
    });    
}

function drawVisualization(dataReply) {
    deleteMarkers();
    var r;
    var operation = document.getElementById('operation').value;

    for(index in dataReply) {
        console.log(operation);

        if (operation === 'cMean')
            r = math.mean(dataReply[index]['Data']);
        else if (operation === 'cStdDev')
            r = math.std(dataReply[index]['Data']);
        else if (operation === 'cMax')
            r = math.max(dataReply[index]['Data']);
        else if (operation === 'cMin')
            r = math.min(dataReply[index]['Data']);
        else
            r = math.mean(dataReply[index]['Data']);

        var visCircle = new google.maps.Circle({
            strokeColor: '#FF0000',
            strokeOpacity: 0.8,
            strokeWeight: 2,
            fillColor: '#FF0000',
            fillOpacity: 0.35,
            map: map,
            center: {lat: dataReply[index]['Position']['Latitude'], lng: dataReply[index]['Position']['Longitude']},
            radius: r
        });

        mapCircle.push(visCircle);
        visCircle.setMap(map);
    }
}

function drawRectVisualization(dataReply) {
    deleteMarkers();
    var heatMapData = [];

    for(index in dataReply) {
        heatMapData.push({location: new google.maps.LatLng(dataReply[index]['Position']['Latitude'], dataReply[index]['Position']['Longitude']), weight: getMean(dataReply[index]['Data'])*5});
    }

    console.log(heatMapData);

    heatmap = new google.maps.visualization.HeatmapLayer({
      data: heatMapData
    });

    heatmap.set('radius', 50)
    heatmap.setMap(map);
}


//Handle buttons
$("#submit").click(function() {
	if(document.getElementById('regType').value == "pointRegion")
    	formRequest(markers);

    else if(document.getElementById('regType').value == "rectRegion")
    	formRequestRect();
});

$("#refresh").click(function() {
    document.getElementById('fromDate').value = "";
    document.getElementById('toDate').value = "";
    deleteMarkers();

    if (heatmap == 'undefined')
        heatmap.setMap(null);
});