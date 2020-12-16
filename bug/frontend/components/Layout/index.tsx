import React, { FC } from 'react'
import { StylesProvider, useMultiStyleConfig } from '@chakra-ui/react'
// Component
import Header from './Header'
import Main from './Main'
import Footer from './Footer'

const Layout: FC = ({ children }) => (
  <StylesProvider value={useMultiStyleConfig('Layout', {})}>
    <Header />
    <Main>{children}</Main>
    <Footer />
  </StylesProvider>
)

export default Layout
