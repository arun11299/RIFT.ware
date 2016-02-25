
The application enables editing of CONFD YANG instances.

Catalog Panel - loads the NSD and VNFD and PNFD catalogs
Canvas Panel - graphical editor of the relations and networks of NSD and VNFD
Details Panel - schema driven editor of every property in the model
Forwarding Graphs Tray - editing FG and Classifier properties

To get an object to show up in the Details Panel it must be defined in the DescriptorModelMeta.json schema file.

# Schema Driven Editor
 
 - only needs the DescriptorModelMeta.json file to define the JSON to create / edited.

 - To make an object appear in the Details Panel you need to add it to the "containersList" in the DescriptorModelFactor.js class.

# Canvas Panel

 - is coded specifically to enable graphical editing of certain descriptor model elements and is the least flexible
 
 - To make an object "selectable" it must have a data-uid field.
 

