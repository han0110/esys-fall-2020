import React, { FC } from 'react'
import Link from 'next/link'
import { Box } from '@chakra-ui/react'
// Component
import BugIcon from '~/components/Icon/Bug'
import GithubIcon from '~/components/Icon/Github'
// Constant
import { BACKEND_ROUTES, ROUTES } from '~/constants'
// Config
import config from '~/config'

const Header: FC = () => (
  <>
    <Box as="header" position="fixed" top="0" w="100vw" boxShadow="sm">
      <Box
        h="20"
        px={[0, 6]}
        m="auto"
        flex="1"
        display="flex"
        alignItems="center"
        maxW={['90vw', '60rem']}
        fontWeight="700"
      >
        <Link href={ROUTES.HOME}>
          <Box as="a" cursor="pointer">
            <BugIcon w="24" h="100%" color="orange.300" />
          </Box>
        </Link>
        <Box margin="auto" />
        <Box
          as="a"
          href={`${config.backend.endpoint}${BACKEND_ROUTES.OAUTH2_GITHUB}`}
          p="2"
          display="flex"
          alignItems="center"
          borderRadius="lg"
          transition="ease-in-out 0.12s"
          _hover={{
            bg: 'orange.100',
          }}
        >
          <Box>Log In</Box>
          <GithubIcon w="8" h="8" pl="2" />
        </Box>
      </Box>
    </Box>
    <Box h="20" />
  </>
)

export default Header
