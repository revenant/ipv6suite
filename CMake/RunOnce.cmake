SET(IPv4_LINK_DIR ${IPv6Suite_SOURCE_DIR}/IP/IPv4)
SET(RUN_DIR ${IPv6Suite_SOURCE_DIR}/IP/DualStack)

## Create Dualstack links to other dirs 
IF(NOT RUN_ONCE_DUALSTACK_LN)
  SET(RUN_ONCE_DUALSTACK_LN done CACHE INTERNAL "Test variable for DualStack headers & ned")

  FIND_PROGRAM(LN_EXE ln)
  EXEC_PROGRAM(${LN_EXE}  ${IPv6Suite_SOURCE_DIR}/Transport ARGS -s
 			  ${IPv6Suite_SOURCE_DIR}/IP/IPv4/TCP .)
  MARK_AS_ADVANCED(FORCE LN_EXE)

  EXEC_PROGRAM(${LN_EXE}  ${RUN_DIR} ARGS -s
 			  ${IPv4_LINK_DIR}/Interface/basic_consts.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/ControlApps/ControlApp.ned .) 
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/QoS/hook_types.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/Interface/ip_address.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/Interface/IPDatagram.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/Interface/IPInterfacePacket.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/MAC_LLC/PPPModule.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/IPNode/ProcessorAccess.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/IPNode/ProcessorManager.ned .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/MAC_LLC/RoutingTableAccess.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/MAC_LLC/RoutingTable.h .)
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -s
 			 ${IPv4_LINK_DIR}/IPProcessing/IPv4Processing.ned .)
ELSE(NOT RUN_ONCE_DUALSTACK_LN)
#Just during transition period for people who already have configured project
  EXEC_PROGRAM(${LN_EXE} ${RUN_DIR} ARGS -sf
 			 ${IPv4_LINK_DIR}/IPProcessing/IPv4Processing.ned .)
ENDIF(NOT RUN_ONCE_DUALSTACK_LN)
