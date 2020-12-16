import React, { FC } from 'react'
import { Box } from '@chakra-ui/react'

const Main: FC = ({ children }) => (
  <Box
    as="main"
    flex="1"
    w="100vw"
    maxW={['90vw', '60rem']}
    mx="auto"
    px={[0, 6]}
  >
    {children}
  </Box>
)

export default Main
