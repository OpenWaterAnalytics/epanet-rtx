const sourceTypes = {
  'odbc': {
    //subOptions: ["simple", "advanced"], // deprecating simple panel
    subOptions: ["advanced"],
    inputRows: [
      {
        inputType:'text-line',
        key:'name',
        text:'Name',
        placeholder:'SCADA Historian Database User-Defined Name',
        helptext:'User-Defined Label or Description for the Database. Type anything here.'
      },{
        inputType:'select-driver',
        key:'driver',
        text:'ODBC Driver',
        placeholder:'Select ODBC Driver',
        helptext:'If you do not see your ODBC Driver, check that it is installed'
      },{
        inputType:'select-zone',
        key:'zone',
        text:'Query Timezone',
        placeholder:'local',
        options:['local','utc'],
        helptext:'Link can assemble queries in terms of UTC or local time.'
      },{
        inputType:'text-line',
        key:'connectionString',
        text:'Connection',
        placeholder:'Server=192.168.0.1;Port=1433;TDS_Version=7.1;Database=Historian_DB;UID=rtx;PWD=rtx',
        helptext: 'ODBC Connection String. Enter the host, port, database, uid, pwd, and any driver-specific options'
      },{
        inputType: 'text-line',
        key:'meta',
        text: 'Name lookup query',
        placeholder: 'SELECT tagname FROM tag_table ORDER BY tagname ASC',
        helptext: 'A query that will return the list of tag names.'
      },{
        inputType: 'text-area',
        key: 'range',
        text: 'Data lookup query',
        placeholder: 'SELECT date, value, quality FROM tbl WHERE tagname = ? AND date >= ? AND date <= ? ORDER BY date ASC',
        helptext: 'A query that will return three columns: (date,value,quality), with three ? placeholders for the tagname, start-date, and end-date, respectively.'
      }
    ]
  },
  'sqlite': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line'
      },
      {
        key: 'connectionString',
        text: 'File Name',
        placeholder: 'tempfile.sqlite',
        inputType: 'text-line'
      }
    ]
  },
  'opc': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line'
      },
      {
        key: 'connectionString',
        text: 'Endpoint',
        placeholder: 'opc.tcp://192.168.0.100/Devices/',
        inputType: 'text-line'
      }
    ]
  },
  'influx': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line'
      },
      {
        key: 'connectionString',
        text: 'Connection',
        placeholder: 'proto=http&host=linkDB.yourdomain.com&port=8086&db=dbname&u=user&p=pass',
        inputType: 'text-line'
      }
    ]
  },
  'pi': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line'
      },
      {
        key: 'connectionString',
        text: 'Connection',
        placeholder: 'proto=http&host=linkDB.yourdomain.com&port=8086&db=dbname&u=user&p=pass',
        inputType: 'text-line'
      },
      {
        key: 'tagSearchPath',
        text: 'Tag Search Path',
        placeholder: 'Example.OpcDaServer.Sahara.*',
        inputType: 'text-line'
      },
      {
        key: 'conversions',
        text: 'Value Conversions',
        placeholder: 'Inactive=0&Active=1',
        inputType: 'text-line'
      }
    ]
  }
};

const destinationTypes = {
  'influx': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line',
        helptext: 'User-defined label or description. Type anything here.'
      },
      {
        key: 'connectionString',
        text: 'Connection',
        placeholder: 'proto=http&host=linkDB.yourdomain.com&port=8086&db=dest_db&u=user&p=pass',
        inputType: 'text-line',
        helptext: 'If you do not have this information, contact the cloud database manager.'
      }
    ]
  },
  'influx_udp': {
    inputRows: [
      {
        key: 'name',
        text: 'Name',
        placeholder: 'User-defined Name',
        inputType: 'text-line',
        helptext: 'User-defined label or description. Type anything here.'
      },
      {
        key: 'connectionString',
        text: 'Connection',
        placeholder: 'host=http://flux.citilogics.io&port=8089',
        inputType: 'text-line',
        helptext: 'If you do not have this information, contact the cloud database manager.'
      }
    ]
  }
};


const rtxTypes = {
  'odbc': 'ODBC',
  'sqlite': 'SQLite',
  'influx': 'Influx',
  'influx_udp': "Influx-UDP",
  'opc': 'OPC',
  'pi':'OSI-PI'
};



module.exports = {sourceTypes,destinationTypes,rtxTypes}
